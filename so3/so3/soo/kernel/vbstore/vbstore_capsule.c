/*
 * Copyright (C) 2016-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <memory.h>

#include <soo/avz.h>
#include <soo/soo.h>
#include <soo/hypervisor.h>
#include <soo/vbstore.h>
#include <soo/evtchn.h>
#include <soo/vbstore_capsule.h>

#include <soo/debug.h>

static void vbs_s3c_rmdir(const char *dir, const char *node)
{
	vbus_rm(VBT_NIL, dir, node);
}

static void vbs_s3c_mkdir(const char *dir, const char *node)
{
	vbus_mkdir(VBT_NIL, dir, node);
}

static void vbs_s3c_write(const char *dir, const char *node, const char *string)
{
	vbus_write(VBT_NIL, dir, node, string);
}

/*
 * The following vbstore node creation does not require to be within a transaction
 * since the backend has no visibility on these nodes until it gets the DC_TRIGGER_DEV_PROBE event.
 */
static void vbstore_dev_init(unsigned int domID, const char *devname, const char *compat)
{
	char rootname[VBS_KEY_LENGTH]; /* Root name depending on domID */
	char propname[VBS_KEY_LENGTH]; /* Property name depending on domID */
	char devrootname[VBS_KEY_LENGTH];
	unsigned int dir_exists = 0; /* Data used to check if a directory exists */

	DBG("%s: creating vbstore entries for domain %d and dev %s\n", __func__, domID, devname);

	/*
	 * We must check here if the /backend/% entry exists.
	 * If not, this means that there is no backend available for this virtual interface. If so, just abort.
	 */
	strcpy(devrootname, "backend/");
	strcat(devrootname, devname);
	dir_exists = vbus_directory_exists(VBT_NIL, devrootname, "");
	if (!dir_exists) {
		lprintk("Cannot find backend node: %s\n", devrootname);
		BUG();
	}

	/* Virtualized interface of dev config */
	sprintf(propname, "%d", domID);
	vbs_s3c_mkdir("device", propname);

	strcpy(devrootname, "device/%d");

	sprintf(rootname, devrootname, domID);
	vbs_s3c_mkdir(rootname, devname);

	strcat(devrootname, "/");
	strcat(devrootname, devname);

	sprintf(rootname, devrootname, domID); /* "/device/%d/vuart"  */
	vbs_s3c_mkdir(rootname, "0");

	strcat(devrootname, "/0"); /*  "/device/%d/vuart/0"   */

	sprintf(rootname, devrootname, domID);
	vbs_s3c_write(rootname, "state", "1"); /* = VBusStateInitialising */

	vbs_s3c_write(rootname, "backend-id", "0");

	strcpy(devrootname, "backend/");
	strcat(devrootname, devname);
	strcat(devrootname, "/%d/0"); /* "backend/vuart/%d/0" */

	sprintf(propname, devrootname, domID);
	vbs_s3c_write(rootname, "backend", propname);

	/* Create the vbstore entries for the backend side */

	sprintf(propname, "%d", domID);
	strcpy(devrootname, "backend/");
	strcat(devrootname, devname); /* "/backend/vuart"  */

	vbs_s3c_mkdir(devrootname, propname);

	strcat(devrootname, "/%d"); /* "/backend/vuart/%d" */

	sprintf(rootname, devrootname, domID);
	vbs_s3c_mkdir(rootname, "0");

	strcat(devrootname, "/0"); /* "/backend/vuart/%d/state/1" */
	sprintf(rootname, devrootname, domID);
	vbs_s3c_write(rootname, "state", "1");

	strcpy(devrootname, "device/%d/");
	strcat(devrootname, devname);
	strcat(devrootname, "/0"); /* "device/%d/vuart/0" */

	sprintf(propname, devrootname, domID);
	vbs_s3c_write(rootname, "frontend", propname);

	sprintf(propname, "%d", domID);
	vbs_s3c_write(rootname, "frontend-id", propname);

	/* Now, we create the corresponding device on the frontend side */
	strcpy(devrootname, "device/%d/");
	strcat(devrootname, devname);
	strcat(devrootname, "/0"); /* "device/%d/vuart/0" */
	sprintf(propname, devrootname, domID);

	/* Create device structure and gets ready to begin interactions with the backend */
	vdev_probe(propname, compat);
}

static void vbstore_dev_remove(unsigned int domID, const char *devname)
{
	char propname[VBS_KEY_LENGTH]; /* Property name depending on domID */
	char devrootname[VBS_KEY_LENGTH];
	unsigned int dir_exists = 0; /* Data used to check if a directory exists */

	DBG("%s: removing vbstore entries for domain %d\n", __func__, domID);

	/*
	 * We must check here if the /backend/% entry exists.
	 * If not, this means that there is no backend available for this virtual interface. If so, just abort.
	 */
	strcpy(devrootname, "backend/");
	strcat(devrootname, devname);
	dir_exists = vbus_directory_exists(VBT_NIL, devrootname, "");
	if (!dir_exists) {
		DBG0("Cannot find backend node: %s\n", devrootname);
		BUG();
	}

	/* Remove virtualized interface of vuart config */
	sprintf(propname, "%d/", domID);

	strcpy(devrootname, "device/");
	strcat(devrootname, propname);
	strcat(devrootname, devname); /* "/device/vuart/" */

	vbs_s3c_rmdir(devrootname, "0");

	/* Remove the vbstore entries for the backend side */

	sprintf(propname, "/%d", domID);

	strcpy(devrootname, "backend/");
	strcat(devrootname, devname); /* "/backend/vuart/" */
	strcat(devrootname, propname); /* "/backend/vuart/2" */

	vbs_s3c_rmdir(devrootname, "0");
}

/*
 * Remove all vbstore entries related to this capsule.
 */
void remove_vbstore_entries(void)
{
	int fdt_node;
	char rootname[VBS_KEY_LENGTH], entry[VBS_KEY_LENGTH];

	/* Remove the vbstore entries related to the capsule */
	strcpy(rootname, "soo/s3c");
	sprintf(entry, "%d", S3C_domID());

	vbus_rm(VBT_NIL, rootname, entry);

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vuihandler,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: Removing vUIHandler from vbstore...\n", __func__);
		vbstore_dev_remove(S3C_domID(), "vuihandler");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vuart,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: removing vuart from vbstore...\n", __func__);
		vbstore_dev_remove(S3C_domID(), "vuart");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vdummy,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: removing vdummy from vbstore...\n", __func__);
		vbstore_dev_remove(S3C_domID(), "vdummy");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vsenseled,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: removing vsenseled from vbstore...\n", __func__);
		vbstore_dev_remove(S3C_domID(), "vsenseled");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vsensej,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: removing vsensej from vbstore...\n", __func__);
		vbstore_dev_remove(S3C_domID(), "vsensej");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vfbdev,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: removing vfbdev from vbstore...\n", __func__);
		vbstore_dev_remove(S3C_domID(), "vfbdev");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vinput,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: removing vinput from vbstore...\n", __func__);
		vbstore_dev_remove(S3C_domID(), "vinput");
	}
}

/*
 * Populate vbstore with all necessary properties required by the frontend drivers.
 */
void vbstore_devices_populate(void)
{
	int fdt_node;

	DBG0("Populate vbstore with frontend information...\n");

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vdummy,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: init vdummy...\n", __func__);
		vbstore_dev_init(S3C_domID(), "vdummy", "vdummy,frontend");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vuihandler,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: Init vUIHandler...\n", __func__);
		vbstore_dev_init(S3C_domID(), "vuihandler", "vuihandler,frontend");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vuart,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: init vuart...\n", __func__);
		vbstore_dev_init(S3C_domID(), "vuart", "vuart,frontend");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vsenseled,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: init vsenseled...\n", __func__);
		vbstore_dev_init(S3C_domID(), "vsenseled", "vsenseled,frontend");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vsensej,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: init vsensej...\n", __func__);
		vbstore_dev_init(S3C_domID(), "vsensej", "vsensej,frontend");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vlogs,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: init vlogs...\n", __func__);
		vbstore_dev_init(S3C_domID(), "vlogs", "vlogs,frontend");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vfbdev,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: init vfbdev...\n", __func__);
		vbstore_dev_init(S3C_domID(), "vfbdev", "vfbdev,frontend");
	}

	fdt_node = fdt_find_compatible_node(__fdt_addr, "vinput,frontend");
	if (fdt_device_is_available(__fdt_addr, fdt_node)) {
		DBG("%s: init vinput...\n", __func__);
		vbstore_dev_init(S3C_domID(), "vinput", "vinput,frontend");
	}
}

void vbstore_trigger_dev_probe(void)
{
	DBG("%s: sending DC_TRIGGER_DEV_PROBE to the agency...\n", __func__);

	/* Trigger the probe on the backend side. */
	do_sync_dom(DOMID_AGENCY, DC_TRIGGER_DEV_PROBE);
}

/*
 * Prepare the vbstore entries used by this capsule.
 */
void vbstore_s3c_init(void)
{
	vbus_probe_frontend_init();

	/* Initialize the interface to vbstore. */

	vbus_vbstore_init();
}

void vbstore_init_dev_populate(void)
{
	DBG0("Now ready to register vbstore entries\n");

	/* Now, we are ready to register vbstore entries */
	vbstore_devices_populate();

	if (get_S3C_state() == S3C_state_stopped) {
		/*
		 * If the capsule is in S3C_state_booting state, this means that the capsule has been locally injected.
		 * There is no need to adjust the grant tables. The TRIGGER_DEV_PROBE event can be sent.
		 */
		vbstore_trigger_dev_probe();
	}
}
