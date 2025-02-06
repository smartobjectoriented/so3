/*
 * Copyright (C) 2014-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2018 Baptiste Delporte <bonel@bonel.net>
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
#include <errno.h>
#include <types.h>
#include <percpu.h>

#include <asm/io.h>
#include <asm/cacheflush.h>

#include <soo/uapi/soo.h>

#include <avz/evtchn.h>
#include <avz/memslot.h>
#include <avz/keyhandler.h>
#include <avz/domain.h>
#include <avz/sched.h>
#include <avz/debug.h>
#include <avz/console.h>
#include <avz/capsule.h>

/**
 * Return the state of the ME corresponding to the ME_slotID.
 * If the ME does not exist anymore (for example, following a KILL_ME),
 * the state is set to ME_state_dead.
 */
ME_state_t get_ME_state(unsigned int ME_slotID)
{
	if (domains[ME_slotID] == NULL)
		return ME_state_dead;
	else
		return domains[ME_slotID]->avz_shared->dom_desc.u.ME.state;
}

void set_ME_state(unsigned int ME_slotID, ME_state_t state)
{
	domains[ME_slotID]->avz_shared->dom_desc.u.ME.state = state;
}

void shutdown_ME(unsigned int ME_slotID)
{
	struct domain *dom;

	dom = domains[ME_slotID];

	/* Perform a removal of ME */
	dom->is_dying = DOMDYING_dead;
	DBG("Shutdowning slotID: %d - Domain pause nosync ...\n", ME_slotID);

	vcpu_pause(dom);

	DBG("Destroy evtchn if necessary - state: %d\n",
	    get_ME_state(ME_slotID));
	evtchn_destroy(dom);

	DBG("Wiping domain area...\n");

	memset((void *)memslot[ME_slotID].base_vaddr, 0,
	       memslot[ME_slotID].size);

	DBG("Destroying domain structure ...\n");

	domain_destroy(dom);

	DBG("Now resetting domains to NULL.\n");

	/* bye bye dear ME ! */
	domains[ME_slotID] = NULL;

	/* Reset the slot availability */
	put_ME_slot(ME_slotID);
}

/**
 * Return the descriptor of a domain (agency or ME).
 * A size of 0 means there is no ME in the slot.
 */
void get_dom_desc(uint32_t slotID, dom_desc_t *dom_desc)
{
	/* Check for authorization... (to be done) */

	/*
	 * If no ME is present in the slot specified by slotID, we assign a size of 0 in the ME descriptor.
	 * We presume that the slotID of agency is never free...
	 */

	if ((slotID > 1) && !memslot[slotID].busy)
		dom_desc->u.ME.size = 0;
	else
		/* Copy the content to the target desc */
		memcpy(dom_desc, &domains[slotID]->avz_shared->dom_desc,
		       sizeof(dom_desc_t));
}

/**
 * SOO hypercall processing.
 */
void do_avz_hypercall(avz_hyp_t *args)
{
	struct domain *dom;

	/* Dispatch the hypercall to the appropriate handler
	 * or do the local processing.
	 */

	switch (args->cmd) {
	case AVZ_CONSOLE_IO_OP:
		do_console_io(&args->u.avz_console_io_args.console);
		break;

	case AVZ_DOMAIN_CONTROL_OP:
		do_domctl(&args->u.avz_domctl_args.domctl);
		break;

	case AVZ_EVENT_CHANNEL_OP:
		do_event_channel_op(args);
		break;

	case AVZ_GRANT_TABLE_OP:
		do_gnttab(&args->u.avz_gnttab_args.gnttab_op);
		break;

	case AVZ_GET_DOM_DESC:
		get_dom_desc(args->u.avz_dom_desc_args.slotID,
			     &args->u.avz_dom_desc_args.dom_desc);
		break;

	case AVZ_ME_READ_SNAPSHOT:
		read_ME_snapshot(args);
		break;

	case AVZ_ME_WRITE_SNAPSHOT:
		write_ME_snapshot(args);
		break;

	case AVZ_INJECT_ME:
		inject_me(args);
		break;

	case AVZ_DC_EVENT_SET:
		/*
		 * AVZ_DC_SET is used to assign a new dc_event number in the (target) domain shared info page.
		 * This has to be done atomically so that if there is still a "pending" value in the field,
		 * the hypercall must return with -BUSY; in this case, the caller has to busy-loop (using schedule preferably)
		 * until the field gets free, i.e. set to DC_NO_EVENT.
		 */

		dom = domains[args->u.avz_dc_event_args.domID];
		BUG_ON(dom == NULL);

		/* The shared info page is set as non cacheable, i.e. if a CPU tries to update it, it becomes visible to other CPUs */
		if (atomic_cmpxchg(&dom->avz_shared->dc_event, DC_NO_EVENT,
				   args->u.avz_dc_event_args.dc_event) !=
		    DC_NO_EVENT)
			args->u.avz_dc_event_args.state = -EBUSY;
		else
			args->u.avz_dc_event_args.state = ESUCCESS;
		break;

	case AVZ_KILL_ME:

		shutdown_ME(args->u.avz_kill_me_args.slotID);

		break;

	case AVZ_GET_ME_STATE:
		args->u.avz_me_state_args.state =
			get_ME_state(args->u.avz_me_state_args.slotID);
		break;

	case AVZ_SET_ME_STATE: {
		set_ME_state(args->u.avz_me_state_args.slotID,
			     args->u.avz_me_state_args.state);
		break;
	}

	default:
		printk("%s: Unrecognized hypercall: %d\n", __func__, args->cmd);
		BUG();
		break;
	}

	flush_dcache_all();
}

void soo_activity_init(void)
{
	DBG("Setting SOO avz up ...\n");
}
