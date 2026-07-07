/*
 * Copyright (c) 2017-2026 REDS Institute, HEIG-VD
 * Author: Daniel Rossier <daniel.rossier@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <common.h>
#include <heap.h>
#include <memory.h>
#include <list.h>
#include <ctype.h>
#include <vfs.h>
#include <log.h>
#include <hashmap.h>

#include <asm/setup.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/serial.h>
#include <device/irq.h>
#include <device/timer.h>
#include <device/ramdev.h>

/*
 * Device status strings
 */
static char *__dev_state_str[] = {
	"unknown",
	"disabled",
	"init pending",
	"initialized",
};

/*
 * List of all dev_t entries of the kernel
 */
static LIST_HEAD(devices);

/*
 * A list of registered devices.
 *
 * A device is registered with the dev_register function from its driver file.
 * It is registered so that it can be accessed e.g. by the VFS using the
 * dev_get_fops function.
 */
static LIST_HEAD(registered_dev);

char *dev_state_str(dev_status_t status)
{
	return __dev_state_str[status];
}

/*
 * Get a dev_t entry based on the compatible string
 */
void *find_device(const char *compat)
{
	dev_t *dev;

	list_for_each_entry(dev, &devices, list)
		if (!strcmp(dev->compatible, compat))
			return dev; /* So far, we take the first match. */

	return NULL;
}

/*
 * Check if a certain node has the property "status" and check for the availability.
 * Only "ok" means a valid device.
 */
bool fdt_device_is_available(void *fdt_addr, int node_offset)
{
	const struct fdt_property *prop;
	int prop_len;

	if (node_offset == -1)
		return false;

	prop = fdt_get_property(fdt_addr, node_offset, "status", &prop_len);

	if (prop) {
		if (!strcmp(prop->data, "disabled"))
			return false;
		else if (!strcmp(prop->data, "ok"))
			return true;
	}
	return false;
}

/*
 * Descriptor stored in the driver hash map: it ties a compatible string (the
 * map key) to the matching driver's init function and its initcall level.
 */
struct driver_match {
	int (*init)(dev_t *dev, int fdt_offset);
	int level;
};

/*
 * Read the content of a device tree and associate a generic device info structure to each
 * relevant entry.
 *
 * So far, the device tree can have only one level of subnode (meaning that the root can contain only
 * nodes at the same level. Managing further sub-node levels require to adapt kernel/fdt.c
 *
 */
void parse_dtb(void *fdt_addr)
{
	unsigned int drivers_count[INITCALLS_LEVELS];
	driver_initcall_t *driver_entries[INITCALLS_LEVELS];
	struct list_head pending[INITCALLS_LEVELS];
	struct driver_match *matches, *match;
	struct hashmap *driver_map;
	dev_t *dev, *tmp;
	int i, level, ret, m;
	int offset, new_off;
	int total_drivers;

	drivers_count[CORE] = ll_entry_count(driver_initcall_t, core);
	driver_entries[CORE] = ll_entry_start(driver_initcall_t, core);

	drivers_count[POSTCORE] = ll_entry_count(driver_initcall_t, postcore);
	driver_entries[POSTCORE] = ll_entry_start(driver_initcall_t, postcore);

	LOG_DEBUG("%s: # entries for core drivers : %d\n", __func__, drivers_count[CORE]);
	LOG_DEBUG("%s: # entries for postcore drivers : %d\n", __func__, drivers_count[POSTCORE]);
	LOG_DEBUG("Now scanning the device tree to retrieve all devices...\n");

	total_drivers = drivers_count[CORE] + drivers_count[POSTCORE];

	/*
	 * Index every registered driver by its compatible string so that each
	 * device tree node can be matched in O(1). Building the map is O(M) over
	 * the M drivers and the single device tree scan below is O(N) over the N
	 * nodes, turning the former O(N * M) nested loop into an O(N + M) parsing.
	 */
	driver_map = hashmap_create(total_drivers);
	ASSERT(driver_map != NULL);

	matches = total_drivers ? (struct driver_match *) malloc(total_drivers * sizeof(*matches)) : NULL;
	ASSERT(total_drivers == 0 || matches != NULL);

	for (level = 0; level < INITCALLS_LEVELS; level++)
		INIT_LIST_HEAD(&pending[level]);

	/*
	 * Insert postcore drivers before core ones: hashmap_put() overwrites on a
	 * duplicate key, so a compatible shared by both levels ends up mapped to
	 * its core driver, preserving the core-before-postcore precedence.
	 */
	m = 0;
	for (level = INITCALLS_LEVELS - 1; level >= 0; level--) {
		for (i = 0; i < drivers_count[level]; i++) {
			matches[m].init = driver_entries[level][i].init;
			matches[m].level = level;
			hashmap_put(driver_map, driver_entries[level][i].compatible, &matches[m]);
			m++;
		}
	}

	/*
	 * Scan the device tree once. A single scratch entry probes every node --
	 * get_dev_info() resets it on each call -- and is only kept (and replaced
	 * by a fresh allocation) when the node matches a driver. Matched nodes are
	 * queued on the list of their initcall level so that the initialization
	 * below can honour the core-before-postcore ordering.
	 */
	dev = (dev_t *) malloc(sizeof(dev_t));
	ASSERT(dev != NULL);

	offset = 0;
	while ((new_off = get_dev_info(fdt_addr, offset, "*", dev)) != -1) {
		offset = new_off;

		if (!fdt_device_is_available(fdt_addr, new_off))
			continue;

		match = (struct driver_match *) hashmap_get(driver_map, dev->compatible);
		if (!match)
			continue;

		list_add_tail(&dev->list, &pending[match->level]);

		dev = (dev_t *) malloc(sizeof(dev_t));
		ASSERT(dev != NULL);
	}

	/* The last scratch buffer was never consumed. */
	free(dev);

	/*
	 * Initialize the matched devices, core drivers before postcore ones and in
	 * device tree order within each level, then move them to the global list.
	 */
	for (level = 0; level < INITCALLS_LEVELS; level++) {
		list_for_each_entry_safe(dev, tmp, &pending[level], list) {
			match = (struct driver_match *) hashmap_get(driver_map, dev->compatible);

			LOG_DEBUG("Found compatible:    %s\n", dev->compatible);
			LOG_DEBUG("    Status:          %s\n", dev_state_str(dev->status));
			LOG_DEBUG("    Initcall level:  %d\n", level);

			if (dev->status == STATUS_INIT_PENDING) {
				ret = match->init(dev, dev->offset_dts);
				BUG_ON(ret);

				dev->status = STATUS_INITIALIZED;
			}

			list_del(&dev->list);
			list_add_tail(&dev->list, &devices);
		}
	}

	if (matches)
		free(matches);

	hashmap_free(driver_map);
}

/* Register a device. Usually called from the device driver. */
void devclass_register(dev_t *dev, struct devclass *devclass)
{
	devclass->dev = dev;
	INIT_LIST_HEAD(&devclass->list);

	list_add(&devclass->list, &registered_dev);
}

/* Gets the indexth registered devclass or NULL if index is too big */
struct devclass *devclass_get_by_index(size_t index)
{
	struct devclass *cur_dev;
	size_t i;

	i = 0;
	list_for_each_entry(cur_dev, &registered_dev, list) {
		if (i == index) {
			return cur_dev;
		}
		++i;
	}
	return NULL;
}

/*
 * Get the cdev of a registered device using the given filename. The vfs_type
 * is also set to the proper value.
 *
 * A device filename has the following format:
 *   /dev/<dev-class>[dev-id]
 *   e.g. /dev/fb0, /dev/input1
 *
 * If dev-id is not specified, there will be no digit appended to the <dev-class>.
 *
 * Note: the given `filename' must not include the /dev/ prefix.
 */
struct devclass *devclass_by_filename(const char *filename)
{
	uint32_t dev_id;
	char *dev_id_s;
	size_t dev_class_len;
	struct devclass *cur_dev;

	/* Find the beginning of the device id string. */
	dev_id_s = (char *) filename;
	while (*dev_id_s && !isdigit(*dev_id_s))
		dev_id_s++;

	if (dev_id_s == filename) {
		lprintk("%s: no device class specified.\n", __func__);
		return NULL;
	}

	/*
	 * Get the device id. If dev_id_s is NULL then 0 should be returned.
	 * TODO simple_strtox functions are deprecated.
	 */
	if (*dev_id_s)
		dev_id = (uint32_t) simple_strtoul(dev_id_s, NULL, 10);
	else
		dev_id = 0;

	/* Get the device class length. */
	dev_class_len = dev_id_s - filename;

	/* Loop through registered_dev. */

	list_for_each_entry(cur_dev, &registered_dev, list) {
		/*
		 * We compare the lengths and use strncmp to compare only the
		 * device class part of `filename'.
		 */
		if ((strlen(cur_dev->class) == dev_class_len) && !strncmp(filename, cur_dev->class, dev_class_len)) {
			if ((dev_id >= cur_dev->id_start) && (dev_id <= cur_dev->id_end))
				return cur_dev;
		}
	}

	lprintk("%s: device not found.\n", __func__);

	return NULL;
}

/*
 * Get the device id of a specific fd.
 * Returns by-default id 0 if no number is specified at the end of the class name.
 */
int devclass_fd_to_id(int fd)
{
	char *pos;
	int val;

	pos = vfs_get_filename(fd);

	while (*pos) {
		if (isdigit(*pos)) {
			/* Found a number */
			val = simple_strtoul(pos, NULL, 10);
			return val;
		} else
			/* Otherwise, move on to the next character. */
			pos++;
	}

	return -1;
}

struct devclass *devclass_by_fd(int fd)
{
	return devclass_by_filename(vfs_get_filename(fd) + DEV_PREFIX_LEN);
}

/*
 * Get the fops of a registered device using the given filename. The vfs_type
 * is also set to the proper value.
 *
 */
struct file_operations *devclass_get_fops(const char *filename, uint32_t *vfs_type)
{
	struct devclass *cdev;

	cdev = devclass_by_filename(filename);
	if (!cdev)
		return NULL;

	*vfs_type = cdev->type;

	return cdev->fops;
}

/*
 * Main device initialization function.
 */
void devices_init(void)
{
	/* Interrupt management subsystem initialization */
	irq_init();

	boot_stage = BOOT_STAGE_IRQ_INIT;

	serial_init();

	timer_dev_init();

#ifdef CONFIG_ROOTFS_RAMDEV
	/* Get possible ram device (aka initrd loaded from U-boot) */
	ramdev_init();
#endif

	/* Pare the associated dtb to initialize all devices */
	parse_dtb(__fdt_addr);
}
