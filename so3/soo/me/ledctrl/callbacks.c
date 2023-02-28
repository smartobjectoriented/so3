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

#include <soo/dev/vsenseled.h>

#include <me/ledctrl.h>

static LIST_HEAD(visits);

/* Reference to the shared content helpful during synergy with other MEs */
sh_ledctrl_t *sh_ledctrl;

static volatile bool full_initd = false;

struct completion upd_lock;

/* Protecting variables between domcalls and the active context */
static spinlock_t propagate_lock;

/**
 * Prepare to be propagated
 */
void propagate(void) {

	spin_lock(&propagate_lock);

	/* Initialize the journey */
	clear_hosts(&visits);

	sh_ledctrl->need_propagate = true;

	spin_unlock(&propagate_lock);

}

/**
 * PRE-ACTIVATE
 *
 * Should receive local information through args
 */
int cb_pre_activate(soo_domcall_arg_t *args) {
	agency_ctl_args_t agency_ctl_args;
	host_entry_t *host_entry;

	/* Retrieve the agency UID of the Smart Object on which the ME is about to be activated. */
	agency_ctl_args.cmd = AG_AGENCY_UID;
	args->__agency_ctl(&agency_ctl_args);

	sh_ledctrl->me_common.here = agency_ctl_args.u.agencyUID;

	host_entry = find_host(&visits, sh_ledctrl->me_common.here);
	if (host_entry)
		/* We already visited this place. */
		set_ME_state(ME_state_killed); /* Will be removed by the agency */
	else
		new_host(&visits, sh_ledctrl->me_common.here, NULL, 0);

	return 0;
}

/**
 * PRE-PROPAGATE
 *
 * The callback is executed in first stage to give a chance to a resident ME to stay or disappear, for example.
 */
int cb_pre_propagate(soo_domcall_arg_t *args) {

	pre_propagate_args_t *pre_propagate_args = (pre_propagate_args_t *) &args->u.pre_propagate_args;

	if (!full_initd) {
		pre_propagate_args->propagate_status = PROPAGATE_STATUS_NO;
		return 0;
	}

	spin_lock(&propagate_lock);

	pre_propagate_args->propagate_status = (sh_ledctrl->need_propagate ? PROPAGATE_STATUS_YES : PROPAGATE_STATUS_NO);

	/* To be killed - only one propagation */
	if (!pre_propagate_args->propagate_status && (get_ME_state() == ME_state_dormant))
		set_ME_state(ME_state_killed);

	/* Only once */
	sh_ledctrl->need_propagate = false;

	spin_unlock(&propagate_lock);

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
	cooperate_args_t *cooperate_args = (cooperate_args_t *) &args->u.cooperate_args;
	agency_ctl_args_t agency_ctl_args;
	sh_ledctrl_t *incoming_sh_ledctrl;
	uint32_t pfn;
	LIST_HEAD(incoming_hosts);

	switch (cooperate_args->role) {
	case COOPERATE_INITIATOR:

		/*
		 * If we are alone in this smart object, we stay here.
		 * The post_activate callback will update the LED.
		 */
		if (cooperate_args->alone) {

			/* No LED currently switched on. */
			sh_ledctrl->local_nr = -1;

			new_host(&visits, sh_ledctrl->me_common.here, NULL, 0);

			sh_ledctrl->me_common.origin = sh_ledctrl->me_common.here;

			complete(&upd_lock);

			return 0;
		}

		/* Collaboration with the target ME */

		/* Update the list of hosts */
		sh_ledctrl->me_common.soohost_nr = concat_hosts(&visits, (uint8_t *) sh_ledctrl->me_common.soohosts);

		agency_ctl_args.u.cooperate_args.pfn = phys_to_pfn(virt_to_phys_pt((addr_t) sh_ledctrl));
		agency_ctl_args.u.cooperate_args.slotID = ME_domID(); /* Will be copied in initiator_cooperate_args */

		/* This pattern enables the cooperation with the target ME */

		agency_ctl_args.cmd = AG_COOPERATE;
		agency_ctl_args.slotID = cooperate_args->u.target_coop.slotID;

		/* Perform the cooperate in the target ME */
		args->__agency_ctl(&agency_ctl_args);

		/* Can be dormant or be killed...*/

		/*
		 * If we reach the smart object initiator, we can disappear, otherwise we keep propagating once.
		 */
		if (sh_ledctrl->initiator == sh_ledctrl->me_common.here)

			set_ME_state(ME_state_killed);

		else if (get_ME_state() != ME_state_killed) {

			set_ME_state(ME_state_dormant);

			spin_lock(&propagate_lock);
			sh_ledctrl->need_propagate = true;
			spin_unlock(&propagate_lock);
		}

		break;

	case COOPERATE_TARGET:

		/* Map the content page of the incoming ME to retrieve its data. */
		pfn = cooperate_args->u.initiator_coop.pfn;
		incoming_sh_ledctrl = (sh_ledctrl_t *) io_map(pfn_to_phys(pfn), PAGE_SIZE);

		/* Are we cooperating with the resident or another (migrating) ME ? */
		if (get_ME_state() == ME_state_dormant) {

			/* If the two MEs are issued from the same SOO origin, hence we can merge
			 * the list of visited hosts and kill the other, no matter if we have more or less
			 * visited hosts.
			 */
			if (sh_ledctrl->me_common.origin == incoming_sh_ledctrl->me_common.origin) {

				/* Merge the visited hosts in our list and kill the other ME (initiator) */
				expand_hosts(&incoming_hosts, incoming_sh_ledctrl->me_common.soohosts,
						incoming_sh_ledctrl->me_common.soohost_nr);

				merge_hosts(&visits, &incoming_hosts);
				clear_hosts(&incoming_hosts);

				/* Kill the ME */
				agency_ctl_args.cmd = AG_KILL_ME;
				agency_ctl_args.slotID = cooperate_args->u.initiator_coop.slotID;
				args->__agency_ctl(&agency_ctl_args);

			}

		} else {

			sh_ledctrl->incoming_nr = incoming_sh_ledctrl->local_nr;

			sh_ledctrl->initiator = incoming_sh_ledctrl->initiator;

			complete(&upd_lock);
		}

		io_unmap((addr_t) incoming_sh_ledctrl);
		break;

	default:
		lprintk("Cooperate: Bad role %d\n", cooperate_args->role);
		BUG();
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

	/* Allocate the shared page. */
	sh_ledctrl = (sh_ledctrl_t *) get_contig_free_vpages(1);

	/* Initialize the shared content page used to exchange information between other MEs */
	memset(sh_ledctrl, 0, PAGE_SIZE);

	init_completion(&upd_lock);

	spin_lock_init(&propagate_lock);

	sh_ledctrl->local_nr = -1;
	sh_ledctrl->incoming_nr = -1;

	/* Set the SPAD capabilities (currently not used) */
	memset(&get_ME_desc()->spad, 0, sizeof(spad_t));

	full_initd = true;
}


