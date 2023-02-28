/*
 * Copyright (C) 2016-2020 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2016-2020 David Truan <david.truan@heig-vd.ch>
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

#include <mutex.h>
#include <delay.h>
#include <timer.h>
#include <heap.h>
#include <memory.h>

#include <apps/upgrader.h>

#include <soo/avz.h>
#include <soo/gnttab.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/debug.h>


/* Null agency UID to check if an agency UID is valid */
agencyUID_t null_agencyUID = {
	.id = { 0 }
};

/* My agency UID */
agencyUID_t my_agencyUID = {
	.id = { 0 }
};

/* Bool telling that at least 1 post-activate has been performed */
bool post_activate_done = false;

struct completion compl;
mutex_t lock1, lock2;

extern void *localinfo_data;

upgrade_versions_args_t agency_versions;

timer_t timer;
/* 
 * Retrieve all versions of agency components.
 */
static int retrieve_agency_versions(void) {

	vbus_scanf(VBT_NIL, "soo", "itb-version", "%u", &agency_versions.itb);
	vbus_scanf(VBT_NIL, "soo", "rootfs-version", "%u", &agency_versions.rootfs);
	vbus_scanf(VBT_NIL, "soo", "uboot-version", "%u", &agency_versions.uboot);

	return 0;
}

upgrade_versions_args_t get_agency_versions(void) {
	return agency_versions;
}

/*
 * The main application of the ME is executed right after the bootstrap. It may be empty since activities can be triggered
 * by external events based on frontend activities.
 */
int main_kernel(void *args) {
	agency_ctl_args_t agency_ctl_args;

	printk("SOO Mobile Entity booting ...\n");

	avz_shared_info->dom_desc.u.ME.spad.valid = true;

	soo_guest_activity_init();

	callbacks_init();

	/* Initialize the Vbus subsystem */
	vbus_init();

	gnttab_init();

	vbstore_init_dev_populate();

	agency_ctl_args.cmd = AG_AGENCY_UID;
	if (agency_ctl(&agency_ctl_args) < 0)
		BUG();

	memcpy(&my_agencyUID, &agency_ctl_args.u.agencyUID_args.agencyUID, SOO_AGENCY_UID_SIZE);
	DBG("Agency UID: "); DBG_BUFFER(&my_agencyUID, SOO_AGENCY_UID_SIZE);

	avz_shared_info->dom_desc.u.ME.spad.valid = true;

	printk("SO3  Mobile Entity -- main app v2.0 -- Copyright (c) 2016-2020 REDS Institute (HEIG-VD)\n\n");

	DBG("ME running as domain %d\n", ME_domID());

	retrieve_agency_versions();

	return 0;
}
