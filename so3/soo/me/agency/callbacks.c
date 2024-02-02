/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) March 2018 Baptiste Delporte <bonel@bonel.net>
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

#include <asm/mmu.h>

#include <memory.h>
#include <sync.h>

#include <apps/upgrader.h>

#include <soo/avz.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/debug.h>

#define MAX_NO_UPGRADE 	4
#define MAX_MIGRATION  	10

/* Localinfo buffer used during cooperation processing */
void *localinfo_data;

/* Flags used to monitor the upgrade process */
static bool upgrade_saved = false;
static bool upgrade_done = false;

/* Counter to decide whether or not we should keep migrate or kill ourself */
static uint32_t no_upgrade_count = 0;
static uint32_t live_count = 0;
static uint32_t migration_count = 0;

/*
 * Check if the Agency we migrated on needs to be upgraded. 
 * 
 * Return: true if Agency needs upgrade, false if not
 */
static bool check_agency_needs_upgrade(void) {
	upgrade_versions_args_t versions = get_agency_versions();

	lprintk("Agency (current) version numbers: ITB: %u, ROOTFS: %u\n", versions.itb, versions.rootfs);
	lprintk("Upgrade version numbers: ITB: %u, ROOTFS: %u\n", (uint32_t)(*(upgrade_image + 4)), (uint32_t)(*(upgrade_image + 12)));

	/* The version numbers are located on the bytes 4 to 15 of the upgrade image */
	if ((uint32_t)(*(upgrade_image + 4)) > versions.itb ||
	    (uint32_t)(*(upgrade_image + 8)) > versions.uboot ||
	    (uint32_t)(*(upgrade_image + 12)) > versions.rootfs) {
		lprintk("SOO.agency: the agency needs to be upgraded.\n");
		return true;
	}

	return false;
}

/**
 * PRE-ACTIVATE
 *
 * Should receive local information through args
 */
int cb_pre_activate(soo_domcall_arg_t *args) {
	agency_ctl_args_t agency_ctl_args;
	char target_soo_name[SOO_NAME_SIZE];

	DBG(">> ME %d: cb_pre_activate..\n", ME_domID());

	/* Retrieve the name of the Smart Object on which the ME has migrated */
	agency_ctl_args.cmd = AG_SOO_NAME;
	args->__agency_ctl(&agency_ctl_args);
	strcpy(target_soo_name, (const char *)agency_ctl_args.u.soo_name_args.soo_name);

	return 0;
}

/**
 * PRE-PROPAGATE
 *
 * The callback is executed in first stage to give a chance to a resident ME to stay or disappear, for example.
 */
int cb_pre_propagate(soo_domcall_arg_t *args) {

	pre_propagate_args_t *pre_propagate_args = (pre_propagate_args_t *)&args->u.pre_propagate_args;

	DBG(">> ME %d: cb_pre_propagate...\n", ME_domID());
	if (upgrade_done) {
		/* Enable migration - here, we migrate MAX_MIGRATION times before being killed. 
		   We also are killed if no upgrade was done over the 4 last migrations */
		if ((get_ME_state() != ME_state_dormant)
		|| (migration_count != MAX_MIGRATION)
		|| (no_upgrade_count != MAX_NO_UPGRADE)) {
			pre_propagate_args->propagate_status = 1;
			migration_count++;
		} else {
			
			/* Reset the state flags */
			upgrade_done = false;
			upgrade_saved = false;
			set_ME_state(ME_state_killed);
		}

		live_count++;
	}
	return 0;
}

/**
 * Kill domcall - if another ME tries to kill us.
 */
int cb_kill_me(soo_domcall_arg_t *args) {

	DBG(">> ME %d: cb_kill_me...\n", ME_domID());

	/* Do we accept to be killed? yes... */
	set_ME_state(ME_state_killed);

	return 0;
}

/**
 * PRE_SUSPEND
 *
 * This callback is executed right before suspending the state of frontend drivers, before migrating
 *
 * Returns 0 if no propagation to the user space is required, 1 otherwise
 */
int cb_pre_suspend(soo_domcall_arg_t *args) {
	DBG(">> ME %d: cb_pre_suspend...\n", ME_domID());

	/* No propagation to the user space */
	return 0;
}

/**
 * COOPERATE
 *
 * This callback is executed when an arriving ME (initiator) decides to cooperate with a residing ME (target).
 */
int cb_cooperate(soo_domcall_arg_t *args) {
	agency_ctl_args_t agency_ctl_args;
	uint32_t upgrade_pfn;

	/* Retrieve and check Agency version numbers */
	if (!check_agency_needs_upgrade()) {
		lprintk("Agency is up-to-date.\n");
		/* Since the upgrade will not happen, it is considered done */
		no_upgrade_count++;
		upgrade_done = true;
		return 0;
	}

	if (!upgrade_saved) {
		/* Reset the no upgrade counter */
		no_upgrade_count = 0;

		/* Retrieve the upgrade image PFN and send it to the Agency */
		upgrade_pfn = phys_to_pfn(virt_to_phys_pt((uint32_t)upgrade_image));
		agency_ctl_args.slotID = ME_domID();
		agency_ctl_args.cmd = AG_AGENCY_UPGRADE;
		agency_ctl_args.u.agency_upgrade_args.buffer_pfn = upgrade_pfn;
		agency_ctl_args.u.agency_upgrade_args.buffer_len = upgrade_image_length;
		args->__agency_ctl(&agency_ctl_args);

		upgrade_saved = true;
	}

	return 0;
}

/**
 * PRE_RESUME
 *
 * This callback is executed right before resuming the frontend drivers, right after ME activation
 *
 * Returns 0 if no propagation to the user space is required, 1 otherwise
 */
int cb_pre_resume(soo_domcall_arg_t *args) {
	DBG(">> ME %d: cb_pre_resume...\n", ME_domID());

	return 0;
}

/**
 * POST_ACTIVATE callback (async)
 */
int cb_post_activate(soo_domcall_arg_t *args) {
	DBG(">> ME %d: cb_post_activate...\n", ME_domID());

	return 0;
}

/**
 * LOCALINFO_UPDATE callback (async)
 *
 * This callback is executed when a localinfo_update DC event is received (normally async).
 *
 * Returns 0 if no propagation to the user space is required, 1 otherwise
 */
int cb_localinfo_update(void) {
	/* Allow to update the migration and killME flags */
	upgrade_done = true;

	return 0;
}

/**
 * FORCE_TERMINATE callback (async)
 *
 * Returns 0 if no propagation to the user space is required, 1 otherwise
 *
 */

int cb_force_terminate(void) {
	DBG(">> ME %d: cb_force_terminate...\n", ME_domID());
	DBG("ME state: %d\n", get_ME_state());

	/* We do nothing particular here for this ME,
	 * however we proceed with the normal termination of execution.
	 */

	set_ME_state(ME_state_terminated);

	return 0;
}

void callbacks_init(void) {

	/* Allocate localinfo */
	localinfo_data = (void *)get_contig_free_vpages(1);

	upgrade_done = false;
	upgrade_saved = false;
	no_upgrade_count = 0;

	/* The ME accepts to collaborate */
	get_ME_desc()->spad.valid = true;

	/* Set the SPAD capabilities */
	memset(get_ME_desc()->spad.caps, 0, SPAD_CAPS_SIZE);
}
