/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2016-2018 Baptiste Delporte <bonel@bonel.net>
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

#include <types.h>
#include <list.h>
#include <string.h>

#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/soo.h>
#include <soo/gnttab.h>
#include <soo/vbstore.h>
#include <soo/vbstore_me.h>

#include <soo/debug.h>
#include <soo/console.h>

/* List of frontend */
struct list_head frontends;

/*
 * Walk through the list of frontend devices and perform an action.
 * When the action returns 1, we stop the walking.
 */
void frontend_for_each(void *data, int (*fn)(struct vbus_device *, void *)) {
	struct list_head *pos, *q;
	struct vbus_device *vdev;

	list_for_each_safe(pos, q, &frontends)
	{
		vdev = list_entry(pos, struct vbus_device, list);

		if (fn(vdev, data) == 1)
			return ;
	}
}

void add_new_dev(struct vbus_device *vdev) {
	list_add_tail(&vdev->list, &frontends);
}

/* device/domID/<type>/<id> => <type>-<id> */
static int frontend_bus_id(char bus_id[VBUS_ID_SIZE], const char *nodename)
{
	/* device/ */
	nodename = strchr(nodename, '/');
	/* domID/ */
	nodename = strchr(nodename+1, '/');
	if (!nodename || strlen(nodename + 1) >= VBUS_ID_SIZE) {
		printk("vbus: bad frontend %s\n", nodename);
		BUG();
	}

	strncpy(bus_id, nodename + 1, VBUS_ID_SIZE);
	if (!strchr(bus_id, '/')) {
		printk("vbus: bus_id %s no slash\n", bus_id);
		BUG();
	}
	*strchr(bus_id, '/') = '-';
	return 0;
}

static void backend_changed(struct vbus_watch *watch)
{
	DBG("Backend changed now: node = %s\n", watch->node);

	vbus_otherend_changed(watch);
}

static char root_name[15];
static char initial_rootname[15];

static struct vbus_type vbus_frontend = {
		.root = "device",
		.get_bus_id = frontend_bus_id,
		.otherend_changed = backend_changed,
};


static int remove_dev(struct vbus_device *vdev, void *data)
{
	if (vdev->vdrv == NULL) {
		/* Skip if driver is NULL, i.e. probe failed */
		return 0;
	}

	/* Remove it from the main list */
	list_del(&vdev->list);

	/* Removal from vbus namespace */
	vbus_dev_remove(vdev);

	return 0;
}

/*
 * Remove a device or all devices present on the bus (if path = NULL)
 */
void remove_devices(void)
{
	frontend_for_each(NULL, remove_dev);
}

static int __device_shutdown(struct vbus_device *vdev, void *data)
{
	/* Removal from vbus namespace */
	vbus_dev_shutdown(vdev);

	return 0;
}

void device_shutdown(void) {
	frontend_for_each(NULL, __device_shutdown);
}

/*
 * In frontend drivers, otherend_id refering to the agency or *realtime* agency is equal to 0.
 */
static void read_backend_details(struct vbus_device *vdev)
{
	vbus_read_otherend_details(vdev, "backend-id", "backend");
}

/*
 * The drivers/vbus_fron have to be registered *before* any registered frontend devices.
 */
void vbus_register_frontend(struct vbus_driver *vdrv)
{
	DBG("Registering driver %s\n", vdrv->name);

	vdrv->read_otherend_details = read_backend_details;
	DBG("__vbus_register_frontend\n");

	vbus_register_driver_common(vdrv);
}

/*
 * We need to adjust names of device after migration as well as
 * removing and recreating watches associated to this device
 */
static int remove_dev_watches(struct vbus_device *vdev, void *data)
{
	char item[80];
	unsigned int domID;
	char *ptr_item;

	/* Process entry "device/<domID>/.. */
	sscanf(vdev->nodename, "device/%d/%s", &domID, item);
	sprintf(vdev->nodename, "device/%d/%s", ME_domID(), item);

	/*
	 * It may happen that the related watches have been already removed during a visit on a smart
	 * object in which the corresponding backend does not exist. It leads to the non-re-creation of
	 * vbstore properties and the watches have not been re-created.
	 * Hence, it is important to check if the watch exists or not.
	 */
	if (vdev->otherend[0] == 0) {
		DBG("Empty dev->otherend, skip free_otherend_watch\n");
		return 0;
	}

	/*
	 * Rename the device name stored in the bus so that we avoid to re-probe a same device (possibly
	 * in a different ME slot.
	 */

	/* Process entry "backend/<type>/<domID>/.. */

	/* backend/ */
	ptr_item = strchr(vdev->otherend, '/');

	/* <type>/ */
	ptr_item = strchr(ptr_item+1, '/');
	ptr_item++;

	sscanf(ptr_item, "%d/%s", &domID, item);
	sprintf(ptr_item, "%d/%s", ME_domID(), item);

	DBG("%s: removing watches for %s\n", __func__, vdev->nodename);

	/* We remove the previous watches since it will be fully recreated during device_populate() */
	free_otherend_watch(vdev, false);

	return 0;
}

/**
 * Called after migration during the resume process.
 */
void postmig_setup(void) {
	/*
	 * First, we need to take care about local watches on vbstore entries to be ready
	 * on property changes.
	 */

	DBG0("Waiting for vbstore dev to be populated\n");

	sprintf(root_name, "%s/%d", initial_rootname, ME_domID());
	DBG("vbus_frontend: %s ... for domID: %d\n", root_name, ME_domID());

	DBG0("Re-adjusting watches....\n");

	/* Walk through all devices and readjust watches */
	frontend_for_each(NULL, remove_dev_watches);

	DBG0("Updating gnttab...\n");

	/* Update gnttab for this slot */
	postmig_gnttab_update();

	/* Re-create the vbstore entries for devices.
	 * During the creation of vbstore entries on the agency side - and after migration - only the existing entries will be updated
	 * Otherwise, the frontend will staid suspended.
	 */

	/* Write the entries related to the ME ID in vbstore */
	vbstore_ME_ID_populate();

	DBG0("Re-creating all vbstore entries & watches for all required (frontend) devices...\n");
	vbstore_devices_populate();

	/*
	 * The grant tables have been adjusted. We can trigger the DC_TRIGGER_DEV_PROBE event after the
	 * next call to vbstore_devices_populate.
	 */
	DBG0("Now triggering dev probe to the backend...\n");
	vbstore_trigger_dev_probe();
}

/*
 * Probe a new device on the frontend bus.
 * Typically called by vbstore_dev_init()
 */
int vdev_probe(char *node, const char *compat) {
	char *type, *pos;
	char target[VBS_KEY_LENGTH];

	DBG("%s: probing device: %s\n", __func__, node);

	strcpy(target, node);
	pos = target;
	type = strsep(&pos, "/");    /* "device/" */
	type = strsep(&pos, "/");    /* "device/<domid>/" */
	type = strsep(&pos, "/");    /* "/device/<domid>/<type>" */

	vbus_dev_changed(node, type, &vbus_frontend, compat);

	return 0;
}

void vbus_probe_frontend_init(void)
{
	DBG0("Initializing...\n");

	/* Preserve the initial root node name */
	strcpy(initial_rootname, vbus_frontend.root);

	/* Re-adjust the root node name with the dom ID */

	sprintf(root_name, "%s/%d", vbus_frontend.root, ME_domID());

	vbus_frontend.root = root_name;

	INIT_LIST_HEAD(&frontends);
}




