/*
 * Copyright (C) 2023 A.Gabriel Catel Torres <arzur.cateltorres@heig-vd.ch>
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
#include <completion.h>

#include <soo/avz.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/debug.h>

#include <me/iuoc.h>

#define BLIND_SPID 0x0020000000000001

static LIST_HEAD(visits);
static LIST_HEAD(known_soo_list);

/* Reference to the shared content helpful during synergy with other MEs */
sh_iuoc_t *sh_iuoc;
sh_blind_t *sh_blind;

struct completion send_data_lock;
atomic_t shutdown;

/**
 * PRE-ACTIVATE
 *
 * Should receive local information through args
 */
void cb_pre_activate(soo_domcall_arg_t *args) {

	agency_ctl_args_t agency_ctl_args;
	agency_ctl_args.cmd = AG_AGENCY_UID;

	DBG(">> ME %d: cb_pre_activate...\n", ME_domID());

	/* Retrieve the agency UID of the Smart Object on which the ME is about to be activated. */
	args->__agency_ctl(&agency_ctl_args);
	sh_iuoc->me_common.here = agency_ctl_args.u.agencyUID;
	DBG(">> ME %d: Agency UID %d\n", ME_domID(), sh_iuoc->me_common.here);
		
#if 0 /* To be implemented... */
	logmsg("[soo:me:SOO.iuoc] ME %d: cb_pre_activate..\n", ME_domID());
#endif

}

/**
 * PRE-PROPAGATE
 *
 * The callback is executed in first stage to give a chance to a resident ME to stay or disappear, for example.
 */
void cb_pre_propagate(soo_domcall_arg_t *args) {

	pre_propagate_args_t *pre_propagate_args = (pre_propagate_args_t *) &args->u.pre_propagate_args;

	DBG(">> ME %d: cb_pre_propagate...\n", ME_domID());

	pre_propagate_args->propagate_status = 0;
}

/**
 * Kill domcall - if another ME tries to kill us.
 */
void cb_kill_me(soo_domcall_arg_t *args) {

	DBG(">> ME %d: cb_kill_me...\n", ME_domID());

	/* Do we accept to be killed? yes... */
	set_ME_state(ME_state_killed);
}

/**
 * PRE_SUSPEND
 *
 * This callback is executed right before suspending the state of frontend drivers, before migrating
 *
 * Returns 0 if no propagation to the user space is required, 1 otherwise
 */
void cb_pre_suspend(soo_domcall_arg_t *args) {
	DBG(">> ME %d: cb_pre_suspend...\n", ME_domID());
}

/**
 * COOPERATE
 *
 * This callback is executed when an arriving ME (initiator) decides to cooperate with a residing ME (target).
 */
void cb_cooperate(soo_domcall_arg_t *args) {

	cooperate_args_t *cooperate_args = (cooperate_args_t *) &args->u.cooperate_args;
	agency_ctl_args_t agency_ctl_args;

	DBG("[IUOC] Cooperate callback!\n");

	switch (cooperate_args->role) {
	case COOPERATE_INITIATOR:

		DBG("[IUOC] Cooperate initiator called !\n");

		if(cooperate_args->u.target_coop.spid == BLIND_SPID) {
			DBG("[IUOC INIT] Cooperate with SOO.blind\n");
			
			agency_ctl_args.u.cooperate_args.pfn = phys_to_pfn(virt_to_phys_pt((addr_t) sh_blind));
			agency_ctl_args.u.cooperate_args.slotID = ME_domID(); /* Will be copied in initiator_cooperate_args */

			/* This pattern enables the cooperation with the target ME */
			agency_ctl_args.cmd = AG_COOPERATE;
			agency_ctl_args.slotID = cooperate_args->u.target_coop.slotID;
			
			/* Perform the cooperate in the target ME */
			args->__agency_ctl(&agency_ctl_args);

			return;
		}
		break;

	case COOPERATE_TARGET:
		break;

	default:
		lprintk("Cooperate: Bad role %d\n", cooperate_args->role);
		BUG();
	}
}

/**
 * PRE_RESUME
 *
 * This callback is executed right before resuming the frontend drivers, right after ME activation
 *
 * Returns 0 if no propagation to the user space is required, 1 otherwise
 */
void cb_pre_resume(soo_domcall_arg_t *args) {
	DBG(">> ME %d: cb_pre_resume...\n", ME_domID());
}

/**
 * POST_ACTIVATE callback (async)
 */
void cb_post_activate(soo_domcall_arg_t *args) {
#if 0
	agency_ctl_args_t agency_ctl_args;
	static uint32_t count = 0;
#endif

	DBG(">> ME %d: cb_post_activate...\n", ME_domID());
}

/**
 * FORCE_TERMINATE callback (async)
 *
 * Returns 0 if no propagation to the user space is required, 1 otherwise
 *
 */

void cb_force_terminate(void) {
	DBG(">> ME %d: cb_force_terminate...\n", ME_domID());
	DBG("ME state: %d\n", get_ME_state());

	atomic_set(&shutdown, 0);

	/* We do nothing particular here for this ME,
	 * however we proceed with the normal termination of execution.
	 */

	set_ME_state(ME_state_terminated);
}

void callbacks_init(void) {
	init_completion(&send_data_lock);
	atomic_set(&shutdown, 1);

	/* Allocate the shared page. */
	sh_iuoc = (sh_iuoc_t *) get_contig_free_vpages(1);
	sh_blind = (sh_blind_t *) get_contig_free_vpages(1);

	/* Initialize the shared content page used to exchange information between other MEs */
	memset(sh_iuoc, 0, PAGE_SIZE);
	memset(sh_blind, 0, PAGE_SIZE);

	/* Set the SPAD capabilities */
	memset(&get_ME_desc()->spad, 0, sizeof(spad_t));
}


