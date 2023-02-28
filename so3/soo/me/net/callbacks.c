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
#include <completion.h>

#include <soo/avz.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/debug.h>

/*
 * ME Description:
 * The ME resides in one (and only one) Smart Object.
 * It is propagated to other Smart Objects until it meets the one with the UID 0x08.
 * At this moment, the ME keeps the info (localinfo_data+1) until the ME comes back to its origin (the running ME instance).
 * The letter is then incremented and the ME is ready for a new trip.
 * The ME must stay dormant in the Smart Objects different than the origin and the one with UID 0x08.
 */

/* Localinfo buffer used during cooperation processing */
void *localinfo_data;


/*
 * migrated_once allows the dormant ME to control its oneshot propagation, i.e.
 * the ME must be broadcast in the neighborhood, then disappear from the smart object.
 */
static uint32_t migration_count = 0;

/**
 * PRE-ACTIVATE
 *
 * Should receive local information through args
 */
int cb_pre_activate(soo_domcall_arg_t *args) {
        char target_soo_name[SOO_NAME_SIZE];

        DBG(">> ME %d: cb_pre_activate..\n", ME_domID());

        /* Retrieve the name of the Smart Object on which the ME has migrated */
        /*agency_ctl_args.cmd = AG_SOO_NAME;
        args->__agency_ctl(&agency_ctl_args);
        strcpy(target_soo_name, (const char *) agency_ctl_args.u.soo_name_args.soo_name);*/


        return 0;
}

/**
 * PRE-PROPAGATE
 *
 * The callback is executed in first stage to give a chance to a resident ME to stay or disappear, for example.
 */
int cb_pre_propagate(soo_domcall_arg_t *args) {

        pre_propagate_args_t *pre_propagate_args = (pre_propagate_args_t *) &args->u.pre_propagate_args;

        DBG(">> ME %d: cb_pre_propagate...\n", ME_domID());

        pre_propagate_args->propagate_status = 0;


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

        unsigned char me_spad_caps[SPAD_CAPS_SIZE];
        unsigned int i;
        void *recv_data;
        uint32_t pfn;
        bool target_found, initiator_found;
        char target_char, initiator_char;

        DBG(">> ME %d: cb_cooperate...\n", ME_domID());

        switch (cooperate_args->role) {
        case COOPERATE_INITIATOR:
                DBG("Cooperate: Initiator %d\n", ME_domID());
                if (cooperate_args->alone)
                        return 0;
                DBG("Cooperate: Multiple\n");

                for (i = 0; i < MAX_ME_DOMAINS; i++) {
                        if (cooperate_args->u.target_coop_slot[i].spad.valid) {

                                memcpy(me_spad_caps, cooperate_args->u.target_coop_slot[i].spad.caps, SPAD_CAPS_SIZE);

                        }
                }


                break;

        case COOPERATE_TARGET:
                DBG("Cooperate: Target %d\n", ME_domID());

                DBG("SPID of the initiator: ");
                DBG_BUFFER(cooperate_args->u.initiator_coop.spid, SPID_SIZE);
                DBG("SPAD caps of the initiator: ");
                DBG_BUFFER(cooperate_args->u.initiator_coop.spad_caps, SPAD_CAPS_SIZE);

                pfn = cooperate_args->u.initiator_coop.pfns.content;
                recv_data = (void *) io_map(pfn_to_phys(pfn), PAGE_SIZE);

                lprintk("## in-cooperate received : %c\n", *((char *) recv_data));

                io_unmap((uint32_t) recv_data);

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
        lprintk("###################### FORCE terminate me %d\n", ME_domID());
        set_ME_state(ME_state_terminated);

        return 0;
}

void callbacks_init(void) {

        /* Allocate localinfo */
        localinfo_data = (void *) get_contig_free_vpages(1);

        /* The ME accepts to collaborate */
        get_ME_desc()->spad.valid = true;

        /* Set the SPAD capabilities */
        memset(get_ME_desc()->spad.caps, 0, SPAD_CAPS_SIZE);
}


