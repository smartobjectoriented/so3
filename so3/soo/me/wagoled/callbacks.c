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

#include <me/wagoled.h>

static LIST_HEAD(visits);
static LIST_HEAD(known_soo_list);


/* Reference to the shared content helpful during synergy with other MEs */
sh_wagoled_t *sh_wagoled;

struct completion send_data_lock;
atomic_t shutdown;
/**
 * PRE-ACTIVATE
 *
 * Should receive local information through args
 */
void cb_pre_activate(soo_domcall_arg_t *args) {

	DBG(">> ME %d: cb_pre_activate...\n", ME_domID());

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
	sh_switch_t *incoming_sh_switch;
	static uint64_t switch_timestamp = 0;
	addr_t pfn;

	switch (cooperate_args->role) {
	case COOPERATE_INITIATOR:

		if (cooperate_args->alone)
			return ;

		break;

	case COOPERATE_TARGET:
		DBG("Cooperate: Target %d\n", ME_domID());
		/* Map the content page of the incoming ME to retrieve its data. */
		pfn = cooperate_args->u.initiator_coop.pfn;
		incoming_sh_switch = (sh_switch_t *) io_map(pfn_to_phys(pfn), PAGE_SIZE);

		if (incoming_sh_switch->timestamp > switch_timestamp) {
			switch_timestamp = incoming_sh_switch->timestamp;
			sh_wagoled->sw_pos = incoming_sh_switch->pos;
			sh_wagoled->sw_status = incoming_sh_switch->status;
			sh_wagoled->switch_event = true;
			incoming_sh_switch->delivered = true;
		}

		io_unmap((addr_t)incoming_sh_switch);
		
		if (sh_wagoled->switch_event) {
			sh_wagoled->switch_event = false;
			complete(&send_data_lock);
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
	sh_wagoled = (sh_wagoled_t *) get_contig_free_vpages(1);

	/* Initialize the shared content page used to exchange information between other MEs */
	memset(sh_wagoled, 0, PAGE_SIZE);

	sh_wagoled->sw_pos = POS_NONE;
	sh_wagoled->sw_status = STATUS_NONE;
	sh_wagoled->switch_event = false;

	/* Set the SPAD capabilities */
	memset(&get_ME_desc()->spad, 0, sizeof(spad_t));
}


