/*
 * Copyright (C) 2017-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#if 0
#define DEBUG
#endif

#include <common.h>
#include <heap.h>
#include <memory.h>
#include <list.h>
#include <errno.h>
#include <ctype.h>
#include <vfs.h>

#include <device/fdt/libfdt.h>

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

char *dev_state_str(dev_status_t status) {
	return __dev_state_str[status];
}

/*
 * Get a dev_t entry based on the compatible string
 */
dev_t *find_device(const char *compat) {
	dev_t *dev;

	list_for_each_entry(dev, &devices, list)
		if (!strcmp(dev->compatible, compat))
			return dev;  /* So far, we take the first match. */

	return NULL;
}

/*
 * Check if a certain node has the property "status" and check for the availability.
 * Only "ok" means a valid device.
 */
bool fdt_device_is_available(int node_offset) {
	const struct fdt_property *prop;
	int prop_len;

	if (node_offset == -1)
		return false;

	prop = fdt_get_property((void *) _fdt_addr, node_offset, "status", &prop_len);

	if (prop) {
		if (!strcmp(prop->data, "disabled"))
			return false;
		else if (!strcmp(prop->data, "ok"))
			return true;

	}
	return false;
}

/*
 * Read the content of a device tree and associate a generic device info structure to each
 * relevant entry.
 *
 * So far, the device tree can have only one level of subnode (meaning that the root can contain only
 * nodes at the same level. Managing further sub-node levels require to adapt kernel/fdt.c
 *
 */
void parse_dtb(void) {
	unsigned int drivers_count[INITCALLS_LEVELS];
	driver_entry_t *driver_entries[INITCALLS_LEVELS];
	dev_t *dev;
	int i, level;
	int offset, new_off;
	bool found;

	drivers_count[CORE] = ll_entry_count(driver_entry_t, core);
	driver_entries[CORE] = ll_entry_start(driver_entry_t, core);

	drivers_count[POSTCORE] = ll_entry_count(driver_entry_t, postcore);
	driver_entries[POSTCORE] = ll_entry_start(driver_entry_t, postcore);	

	DBG("%s: # entries for core drivers : %d\n", __func__, drivers_count[CORE]);
	DBG("%s: # entries for postcore drivers : %d\n", __func__, drivers_count[POSTCORE]);
	DBG("Now scanning the device tree to retrieve all devices...\n");

	for (level = 0; level < INITCALLS_LEVELS; level++) { 
		dev = (dev_t *) malloc(sizeof(dev_t));
		ASSERT(dev != NULL);
		memset(dev, 0, sizeof(dev_t));

		found = false;
		offset = 0;

		while ((new_off = get_dev_info((void *) _fdt_addr, offset, "*", dev)) != -1) {
			if (fdt_device_is_available(new_off)) {
				for (i = 0; i < drivers_count[level]; i++) {

					if (strcmp(dev->compatible, driver_entries[level][i].compatible) == 0) {

						found = true;

						DBG("Found compatible:    %s\n",driver_entries[level][i].compatible);
						DBG("    Compatible:      %s\n", dev->compatible);
						DBG("    Base address:    0x%08X\n", dev->base);
						DBG("    Size:            0x%08X\n", dev->size);
						DBG("    IRQ:             %d\n", dev->irq);
						DBG("    Status:          %s\n", dev_state_str(dev->status));
						DBG("    Initcall level:  %d\n", level);

						if (dev->status == STATUS_INIT_PENDING) {
#ifdef CONFIG_MMU
							/* Perform the mapping in the I/O virtual address space, if necessary (size > 0) */
							if (dev->size > 0) {
								dev->base = io_map(dev->base, dev->size);
								DBG("    Virtual addr: 0x%x\n", dev->base);
							}
#endif

							driver_entries[level][i].init(dev);
							dev->status = STATUS_INITIALIZED;

							list_add_tail(&dev->list, &devices);
						}
						break;
					}
				}
			}
			if (!found)
				free(dev);

			offset = new_off;

			dev = (dev_t *) malloc(sizeof(dev_t));
			ASSERT(dev != NULL);
			memset(dev, 0, sizeof(dev_t));

			found = false;
		}
	}

	/* We have always the last allocation which will not be used */
	free(dev);
}

/*
 * Get the device id of a specific fd.
 * Returns by-default id 0 if no number is specified at the end of the class name.
 */
int devclass_get_id(int fd) {
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

/* Register a device. Usually called from the device driver. */
void devclass_register(dev_t *dev, struct devclass *cdev)
{
	cdev->dev = dev;
	INIT_LIST_HEAD(&cdev->list);

	list_add(&cdev->list, &registered_dev);
}

/*
 * Get the cdev of a registered device using the given filename. The vfs_type
 * is also set to the proper value.
 *
 * A device filename has the following format:
 *   /dev/<dev-class>[dev-id]
 *   e.g. /dev/fb0, /dev/input1
 *
 * If dev-id is not specified, 0 is assumed.
 *
 * Note: the given `filename' must not include the /dev/ prefix.
 */
struct devclass *devclass_get_cdev(const char *filename)
{
	uint32_t dev_id, i;
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
	i = 0;
	list_for_each_entry(cur_dev, &registered_dev, list) {

		/*
		 * We compare the lengths and use strncmp to compare only the
		 * device class part of `filename'.
		 */
		if ((strlen(cur_dev->class) == dev_class_len) && !strncmp(filename, cur_dev->class, dev_class_len)) {

			if ((dev_id == i++) || ((dev_id >= cur_dev->id_start) && (dev_id <= cur_dev->id_end)))
				return cur_dev;
		}
	}

	lprintk("%s: device not found.\n", __func__);

	return NULL;
}

/*
 * Get the fops of a registered device using the given filename. The vfs_type
 * is also set to the proper value.
 *
 */
struct file_operations *devclass_get_fops(const char *filename, uint32_t *vfs_type)
{
	struct devclass *cdev;

	cdev = devclass_get_cdev(filename);
	if (!cdev)
		return NULL;

	*vfs_type = cdev->type;

	return cdev->fops;
}


/*
 * Main device initialization function.
 */
void devices_init(void) {

#ifdef CONFIG_SO3VIRT
	board_setup_post();
#endif
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
	parse_dtb();
}
