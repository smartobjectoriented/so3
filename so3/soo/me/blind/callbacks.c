/*
 * Copyright (C) 2023 A.Gabriel Catel Torres <arzur.cateltorres@heig-vd.ch>
 * Copyright (C) 2014-2022 Daniel Rossier <daniel.rossier@soo.tech>
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
#include <completion.h>

#include <soo/avz.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/debug.h>

#include <me/blind.h>
#include <me/switch.h>

#define SWITCH_SPID 	0x0020000000000004
#define IUOC_SPID 		0x0030000000000003

static LIST_HEAD(visits);
static LIST_HEAD(known_soo_list);

static volatile bool full_initd = false;
static volatile bool originUID_initd = false;

/* Reference to the shared content helpful during synergy with other MEs */
sh_blind_t *sh_blind;
struct completion send_data_lock;
atomic_t shutdown;
spinlock_t propagate_lock;

/**
 * PRE-ACTIVATE
 *
 * Should receive local information through args
 */
void cb_pre_activate(soo_domcall_arg_t *args) {
	agency_ctl_args_t agency_ctl_args;
	host_entry_t *host_entry;

	DBG(">> ME %d: cb_pre_activate...\n", ME_domID());

	/* Retrieve the agency UID of the Smart Object on which the ME is about to be activated. */
	agency_ctl_args.cmd = AG_AGENCY_UID;
	args->__agency_ctl(&agency_ctl_args);

	sh_blind->me_common.here = agency_ctl_args.u.agencyUID;

	if (!originUID_initd) {
		sh_blind->originUID = agency_ctl_args.u.agencyUID;
		originUID_initd = true;
	}

	DBG(">> ME %d: originUID %d\n", ME_domID(), sh_blind->originUID);

	host_entry = find_host(&visits, sh_blind->me_common.here);
	
	if (host_entry) {
		/** If we already visited this host we ask the agency to kill us **/
		DBG(MESWITCH_PREFIX "Host already visited. Killing myself\n");
		set_ME_state(ME_state_killed);
	} else {
		/** We add the host to the visited hosts list **/
		new_host(&visits, sh_blind->me_common.here, NULL, 0);
		DBG(MESWITCH_PREFIX "Adding new host\n");
	}

	agency_ctl_args.cmd = AG_CHECK_DEVCAPS;
	args->__agency_ctl(&agency_ctl_args);

	DBG(">> ME %d: devcaps.class = %d, devcaps.devcaps = %d\n", ME_domID(), 
		agency_ctl_args.u.devcaps_args.class, agency_ctl_args.u.devcaps_args.devcaps);

#if 0 /* To be implemented... */
	logmsg("[soo:me:SOO.wagoled] ME %d: cb_pre_activate..\n", ME_domID());
#endif

}

/**
 * PRE-PROPAGATE
 *
 * The callback is executed in first stage to give a chance to a resident ME to stay or disappear, for example.
 */
void cb_pre_propagate(soo_domcall_arg_t *args) {

	pre_propagate_args_t *pre_propagate_args = (pre_propagate_args_t *) &args->u.pre_propagate_args;
	
	spin_lock(&propagate_lock);

	pre_propagate_args->propagate_status = sh_blind->need_propagate ? PROPAGATE_STATUS_YES : PROPAGATE_STATUS_NO;

	DBG(">> ME %d: cb_pre_propagate = %d\n", ME_domID(), pre_propagate_args->propagate_status);

	/* To be killed - only one propagation, here we set the real propagate_status */
	if (!pre_propagate_args->propagate_status && (get_ME_state() == ME_state_dormant)) {
		set_ME_state(ME_state_killed);
		printk("[BLIND ME] Now killing the ME %d\n", ME_domID());
	}

	sh_blind->need_propagate = false;

	spin_unlock(&propagate_lock);
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

	sh_switch_t *incoming_sh_switch;
	sh_blind_t *incoming_sh_blind;

	/* Keeping this for now, not used and define a strategy to manage timestamps */
	static uint64_t blind_timestamp = 0;
	static uint64_t blind_iuoc_timestamp = 0;

	addr_t pfn;
	agency_ctl_args_t agency_ctl_args;

	switch (cooperate_args->role) {

	case COOPERATE_INITIATOR:
		if (cooperate_args->alone) {
				DBG("We are alone! Continue migration\n");
				
				set_ME_state(ME_state_dormant);

				spin_lock(&propagate_lock);
				sh_blind->need_propagate = true;
				spin_unlock(&propagate_lock);
			}

		DBG("[BLIND] Cooperate initiator called !\n");

		if(cooperate_args->u.target_coop.spid == get_spid()) {
			DBG("[BLIND] Found a SOO.blind, cooperating...\n");

			agency_ctl_args.u.cooperate_args.pfn = phys_to_pfn(virt_to_phys_pt((addr_t) sh_blind));
			agency_ctl_args.u.cooperate_args.slotID = ME_domID(); /* Will be copied in initiator_cooperate_args */

			/* This pattern enables the cooperation with the target ME */
			agency_ctl_args.cmd = AG_COOPERATE;
			agency_ctl_args.slotID = cooperate_args->u.target_coop.slotID;
			
			/* Perform the cooperate in the target ME */
			args->__agency_ctl(&agency_ctl_args);

			set_ME_state(ME_state_dormant);

			spin_lock(&propagate_lock);
			sh_blind->need_propagate = false;
			spin_unlock(&propagate_lock);
		}

		break;

	case COOPERATE_TARGET:
		DBG("[BLIND ME] Cooperate: Target %d\n", ME_domID());
		pfn = cooperate_args->u.initiator_coop.pfn;

		/** Check if we have been reached by a SOO.blind **/
		if(cooperate_args->u.initiator_coop.spid == get_spid()) {
			DBG("[BLIND ME] Cooperation coming from another BLIND\n");

			/* Map the content page of the incoming ME to retrieve its data. */
			incoming_sh_blind = (sh_blind_t *) io_map(pfn_to_phys(pfn), PAGE_SIZE);

			/* Not checking the timestampfor now, define how to do it correctly to match
			   between SO */
			blind_timestamp       = incoming_sh_blind->timestamp;
			sh_blind->direction   = incoming_sh_blind->direction;
			sh_blind->action_mode = incoming_sh_blind->action_mode;

			sh_blind->blind_event = true;
			incoming_sh_blind->delivered = true;

			io_unmap((addr_t) incoming_sh_blind);

			if (sh_blind->blind_event) {
				sh_blind->blind_event = false;
				complete(&send_data_lock);
			}

		} else if(cooperate_args->u.initiator_coop.spid == SWITCH_SPID) {
			DBG("[BLIND ME] Cooperation coming from a SWITCH\n");

			/* Check if we have been reached by a SOO.switch */
			/* Map the content page of the incoming ME to retrieve its data. */
			incoming_sh_switch = (sh_switch_t *) io_map(pfn_to_phys(pfn), PAGE_SIZE);

			/* Not checking the timestampfor now, define how to do it correctly to match
			   between SO */
			blind_timestamp = incoming_sh_switch->timestamp;

			sh_blind->direction   = incoming_sh_switch->pos == POS_LEFT_UP ? BLIND_UP : BLIND_DOWN;
			sh_blind->action_mode = incoming_sh_switch->press == PRESS_LONG ? BLIND_FULL : BLIND_STEP;
			
			sh_blind->blind_event = true;
			incoming_sh_switch->delivered = true;

			io_unmap((addr_t) incoming_sh_switch);

			if (sh_blind->blind_event) {
				sh_blind->blind_event = false;
				complete(&send_data_lock);
			}
		
		} else if(cooperate_args->u.initiator_coop.spid == IUOC_SPID) {
			DBG("[BLIND ME] Cooperation coming from IUOC\n");

			/* Check if we have been reached by a SOO.iuoc */
			incoming_sh_blind = (sh_blind_t *) io_map(pfn_to_phys(pfn), PAGE_SIZE);

			/* Not checking the timestampfor now, define how to do it correctly to match
			   between SO */
			blind_iuoc_timestamp  = incoming_sh_blind->timestamp;
			sh_blind->direction   = incoming_sh_blind->direction;
			sh_blind->action_mode = incoming_sh_blind->action_mode;
			
			io_unmap((addr_t) incoming_sh_blind);

			spin_lock(&propagate_lock);
			sh_blind->need_propagate = true;
			spin_unlock(&propagate_lock);
			
		} else {
			DBG("Shouldn't cooperate... bad ME SPID...\n");
			return ;
		}

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
	printk(">> ME %d: cb_force_terminate...\n", ME_domID());
	DBG("ME state: %d\n", get_ME_state());

	atomic_set(&shutdown, 0);

	printk("ME state: %d\n", get_ME_state());
	
	/* We do nothing particular here for this ME,
	 * however we proceed with the normal termination of execution.
	 */

	set_ME_state(ME_state_terminated);
}

void callbacks_init(void) {

	init_completion(&send_data_lock);
	atomic_set(&shutdown, 1);
	spin_lock_init(&propagate_lock);

	/* Allocate the shared page. */
	sh_blind = (sh_blind_t *) get_contig_free_vpages(1);;

	/* Initialize the shared content page used to exchange information between other MEs */
	memset(sh_blind, 0, PAGE_SIZE);

	sh_blind->blind_event = false;
	sh_blind->direction = BLIND_DIRECTION_NULL;
	sh_blind->action_mode = BLIND_ACTION_MODE_NULL;

	sh_blind->timestamp = 0; 
	
	sh_blind->need_propagate = false;
	sh_blind->delivered = false;

	/* Set the SPAD capabilities */
	memset(&get_ME_desc()->spad, 0, sizeof(spad_t));
	
	full_initd = true;
}


