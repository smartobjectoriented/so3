/*
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

#include <me/switch.h>


#define SOO_BLIND_SPID 			0x0020000000000001
#define SOO_WAGOLED_SPID		0x0020000000000003

static LIST_HEAD(visits);
static LIST_HEAD(known_soo_list);


/* Reference to the shared content helpful during synergy with other MEs */
sh_switch_t *sh_switch;
struct completion send_data_lock;
atomic_t shutdown;

static volatile bool originUID_initd = false;

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

	sh_switch->me_common.here = agency_ctl_args.u.agencyUID;
	if (!originUID_initd) {
		sh_switch->originUID = agency_ctl_args.u.agencyUID;
		originUID_initd = true;
	}
	DBG(">> ME %d: originUID %d\n", ME_domID(), sh_switch->originUID);

	host_entry = find_host(&visits, sh_switch->me_common.here);
	if (host_entry) {
		/** If we already visited this host we ask the agency to kill us **/
		DBG(MESWITCH_PREFIX "Host already visited. Killing myself\n");
		set_ME_state(ME_state_killed);
	} else {
		/** We add the host to the visited hosts list **/
		new_host(&visits, sh_switch->me_common.here, NULL, 0);
		DBG(MESWITCH_PREFIX "Adding new host\n");
	}


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

	agency_ctl_args_t agency_ctl_args;

	DBG(">> ME %d: cb_pre_propagate...\n", ME_domID());

	spin_lock(&propagate_lock);

	pre_propagate_args->propagate_status = sh_switch->need_propagate ? PROPAGATE_STATUS_YES : PROPAGATE_STATUS_NO;

	/* To be killed - only one propagation, here we set the real propagate_status */
	if (!pre_propagate_args->propagate_status && (get_ME_state() == ME_state_dormant)) {
		set_ME_state(ME_state_killed);
		DBG("Now killing the ME %d\n", ME_domID());
	}

	sh_switch->need_propagate = false;

	spin_unlock(&propagate_lock);

	return ;
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
	sh_switch_t *target_sh;
	agency_ctl_args_t agency_ctl_args;

	// lprintk("[soo:me:SOO.blind] ME %d: cb_cooperate...\n", ME_domID());

	switch (cooperate_args->role) {
	case COOPERATE_INITIATOR:

		/** 
		 * If we are alone or we encounter another SOO.switch, we go dormant and continue 
		 * our migration 
		 */
		if (cooperate_args->alone) {
			DBG("We are alone! Continue migration\n");
			
			set_ME_state(ME_state_dormant);

			spin_lock(&propagate_lock);
			sh_switch->need_propagate = true;
			spin_unlock(&propagate_lock);
		}

		/** Check if we encountered another SOO.switch **/
		if(get_spid() == cooperate_args->u.target_coop.spid) {
			DBG(MESWITCH_PREFIX "Found ME switch\n");
			/**
			 * Get the other SOO.switch shared data 
			 */
			target_sh = (sh_switch_t*)io_map(pfn_to_phys(cooperate_args->u.target_coop.pfn), PAGE_SIZE);
			
			/** Check if we are of same type **/
			if (sh_switch->type != target_sh->type) {
				DBG(MESWITCH_PREFIX "We are not the same\n");
				set_ME_state(ME_state_dormant);
				if (sh_switch->delivered) {
					spin_lock(&propagate_lock);
					sh_switch->need_propagate = false;
					spin_unlock(&propagate_lock);
					
					DBG(MESWITCH_PREFIX "Data delivered.\n");
				} else {
					spin_lock(&propagate_lock);
					sh_switch->need_propagate = true;
					spin_unlock(&propagate_lock);

					DBG(MESWITCH_PREFIX "Continuing migration\n");
				}
			} else {

				/** Check if we encountered ourself **/
				if (sh_switch->originUID == target_sh->originUID) {
				/** Check if we are newer **/
					if (sh_switch->timestamp > target_sh->timestamp) {
						/** copy shared data to in target ME **/
						memcpy(target_sh, sh_switch, sizeof(sh_switch_t));
					}
					DBG("Found ourself. Killing initiator ME.\n");
					set_ME_state(ME_state_killed);
				} else {
					/**
				 	* Continue migration 
				 	*/
					set_ME_state(ME_state_dormant);
					spin_lock(&propagate_lock);
					sh_switch->need_propagate = true;
					spin_unlock(&propagate_lock);

					DBG(MESWITCH_PREFIX "Continuing migration");
				}
			}
			io_unmap((addr_t) target_sh);
		
		} else if (cooperate_args->u.target_coop.spid == SOO_BLIND_SPID && sh_switch->type == PTM210) {
			/** Check if target is a SOO.Blind or **/
			DBG(MESWITCH_PREFIX "Cooperate with SOO.blind\n");

			agency_ctl_args.u.cooperate_args.pfn = phys_to_pfn(virt_to_phys_pt((addr_t) sh_switch));
			agency_ctl_args.u.cooperate_args.slotID = ME_domID(); /* Will be copied in initiator_cooperate_args */

			/* This pattern enables the cooperation with the target ME */
			agency_ctl_args.cmd = AG_COOPERATE;
			agency_ctl_args.slotID = cooperate_args->u.target_coop.slotID;
			
			/* Perform the cooperate in the target ME */
			args->__agency_ctl(&agency_ctl_args);


			// set_ME_state(ME_state_dormant);
			// spin_lock(&propagate_lock);
			// sh_switch->need_propagate = false;
			// spin_unlock(&propagate_lock);

			DBG(MESWITCH_PREFIX "Cooperation with blind over!\n");
			return ;

		} else if (cooperate_args->u.target_coop.spid == SOO_WAGOLED_SPID && sh_switch->type == GTL2TW) {
			DBG(MESWITCH_PREFIX "Cooperate with SOO.wagoled\n");

			agency_ctl_args.u.cooperate_args.pfn = phys_to_pfn(virt_to_phys_pt((addr_t) sh_switch));
			agency_ctl_args.u.cooperate_args.slotID = ME_domID(); /* Will be copied in initiator_cooperate_args */

			/* This pattern enables the cooperation with the target ME */
			agency_ctl_args.cmd = AG_COOPERATE;
			agency_ctl_args.slotID = cooperate_args->u.target_coop.slotID;
			
			/* Perform the cooperate in the target ME */
			args->__agency_ctl(&agency_ctl_args);


			set_ME_state(ME_state_dormant);
			spin_lock(&propagate_lock);
			sh_switch->need_propagate = false;
			spin_unlock(&propagate_lock);

			return ;
		} else {
			DBG("We cannot cooperate with this ME: 0x%16X! Continue migration\n", cooperate_args->u.target_coop.spid);
			
			set_ME_state(ME_state_dormant);
			spin_lock(&propagate_lock);
			sh_switch->need_propagate = true;
			spin_unlock(&propagate_lock);

			return ;
		}

		break;

	case COOPERATE_TARGET:
		DBG("Cooperate: Target %d\n", ME_domID());
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
	sh_switch = (sh_switch_t *) get_contig_free_vpages(1);;

	/* Initialize the shared content page used to exchange information between other MEs */
	memset(sh_switch, 0, PAGE_SIZE);

	sh_switch->switch_event = false;
	sh_switch->pos = POS_NONE;
	sh_switch->press = PRESS_NONE;
	sh_switch->status = STATUS_NONE;
	sh_switch->need_propagate = false;
	sh_switch->delivered = false;

#if defined(KNX)
	sh_switch->type = GTL2TW;
#elif defined(ENOCEAN)
	sh_switch->type = PTM210;
#endif

	/* Set the SPAD capabilities */
	memset(&get_ME_desc()->spad, 0, sizeof(spad_t));
}


