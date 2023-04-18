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

#include <avz/uapi/me_access.h>
#include <avz/uapi/soo.h>

#include <avz/migration.h>
#include <avz/memslot.h>
#include <avz/keyhandler.h>
#include <avz/domain.h>
#include <avz/sched.h>
#include <avz/debug.h>

/**
 * Return the state of the ME corresponding to the ME_slotID.
 * If the ME does not exist anymore (for example, following a KILL_ME),
 * the state is set to ME_state_dead.
 */
ME_state_t get_ME_state(unsigned int ME_slotID) {
	if (domains[ME_slotID] == NULL)
		return ME_state_dead;
	else
		return domains[ME_slotID]->avz_shared->dom_desc.u.ME.state;

}

void set_ME_state(unsigned int ME_slotID, ME_state_t state) {
	domains[ME_slotID]->avz_shared->dom_desc.u.ME.state = state;
}

void shutdown_ME(unsigned int ME_slotID)
{
	struct domain *dom;
	struct domain *__current_domain;
	addr_t current_pgtable_paddr;

	dom = domains[ME_slotID];

	/* Perform a removal of ME */
	dom->is_dying = DOMDYING_dead;
	DBG("Shutdowning slotID: %d - Domain pause nosync ...\n", ME_slotID);

	vcpu_pause(dom);

	DBG("Destroy evtchn if necessary - state: %d\n", get_ME_state(ME_slotID));
	evtchn_destroy(dom);

	DBG("Switching address space ...\n");

	__current_domain = current_domain;
	mmu_get_current_pgtable(&current_pgtable_paddr);

	mmu_switch_kernel((void *) idle_domain[smp_processor_id()]->avz_shared->pagetable_paddr);

	memset((void *) __lva(memslot[ME_slotID].base_paddr), 0, memslot[ME_slotID].size);

	set_current_domain(__current_domain);
	mmu_switch_kernel((void *) current_pgtable_paddr);

	DBG("Destroying domain structure ...\n");

	domain_destroy(dom);

	DBG("Now resetting domains to NULL.\n");

	/* bye bye dear ME ! */
	domains[ME_slotID] = NULL;

	/* Reset the slot availability */
	put_ME_slot(ME_slotID);

}

/*
 * Check if some ME need to be killed.
 */
void check_killed_ME(void) {
	int i;

	for (i = MEMSLOT_BASE; i < MEMSLOT_NR; i++) {

		if (memslot[i].busy && (get_ME_state(i) == ME_state_killed))
			shutdown_ME(i);

	}
}

/***** SOO post-migration callbacks *****/

/*
 * Forward a domcall to the ME corresponding to <forward_slotID>
 *
 */
/*
 * agency_ctl()
 *
 * Request a specific action to the agency (issued from a ME for example)
 *
 */
void agency_ctl(agency_ctl_args_t *args)
{
	struct domain *target_dom;
	soo_domcall_arg_t domcall_args;
	int cpu;

	memset(&domcall_args, 0, sizeof(soo_domcall_arg_t));

	/* Prepare the agency ctl args and the domcall args */
	memcpy(&domcall_args.u.agency_ctl_args, args, sizeof(agency_ctl_args_t));
	domcall_args.__agency_ctl = agency_ctl;

	/* Perform a domcall on the target ME */
	DBG("Processing agency_ctl function, cmd=0x%08x\n", args->cmd);

	switch (args->cmd) {

	case AG_KILL_ME:

		/* Performs a domcall in the ME to validate its removal. */

		domcall_args.cmd = CB_KILL_ME;
		target_dom = domains[args->slotID];

		/* Final shutdown of the ME if the state is set to killed will be performed elsewhere,
		 * by the caller.
		 */
		break;

	case AG_AGENCY_UID:
		args->u.agencyUID = domains[0]->avz_shared->dom_desc.u.agency.agencyUID;
		return ;

	case AG_LOCAL_COOPERATE:
		soo_cooperate(args->u.cooperate_args.slotID);
		return;

	case AG_COOPERATE:
		domcall_args.cmd = CB_COOPERATE;

		/* Initiator slotID must be filled by the ME - during the cooperation - through the target slotID */
		domcall_args.u.cooperate_args.u.initiator_coop.slotID = args->u.cooperate_args.slotID;

		/* target_cooperate_args must be provided by the initiator ME during the cooperation */
		domcall_args.u.cooperate_args.u.initiator_coop.pfn = args->u.cooperate_args.pfn;

		/* Transfer the capabilities of the target ME */
		memcpy(&domcall_args.u.cooperate_args.u.initiator_coop.spad, &domains[args->slotID]->avz_shared->dom_desc.u.ME.spad, sizeof(spad_t));

		/* Transfer the SPID of the target ME */
		domcall_args.u.cooperate_args.u.initiator_coop.spid = domains[args->u.cooperate_args.slotID]->avz_shared->dom_desc.u.ME.spid;

		domcall_args.u.cooperate_args.role = COOPERATE_TARGET;
		target_dom = domains[args->slotID];

		break;

	default:

		domcall_args.cmd = CB_AGENCY_CTL;

		target_dom = domains[0]; /* Agency */

	}

	/*
	 * current_mapped_domain is associated to each CPU.
	 */
	cpu = smp_processor_id();

	/* Originating ME */
	domcall_args.slotID = current_domain->avz_shared->domID;

	domain_call(target_dom, DOMCALL_soo, &domcall_args);

	DBG("Ending forward callback now, back to the originater...\n");

	/* Copy the agency ctl args back */
	memcpy(args, &domcall_args.u.agency_ctl_args, sizeof(agency_ctl_args_t));
}

/**
 * Initiate the pre-propagate callback on a ME.
 */
static void soo_pre_propagate(unsigned int slotID, int *propagate_status) {
	soo_domcall_arg_t domcall_args;

	memset(&domcall_args, 0, sizeof(domcall_args));

	domcall_args.cmd = CB_PRE_PROPAGATE;
	domcall_args.__agency_ctl = agency_ctl;

#if 0
	DBG("Pre-propagate callback being issued...\n");
#endif

	domcall_args.slotID = slotID;

	domain_call(domains[slotID], DOMCALL_soo, &domcall_args);

	*propagate_status = domcall_args.u.pre_propagate_args.propagate_status;

	/* If the ME decides itself to be removed */
	check_killed_ME();
}

void soo_pre_activate(unsigned int slotID)
{
	soo_domcall_arg_t domcall_args;

	memset(&domcall_args, 0, sizeof(domcall_args));

	domcall_args.cmd = CB_PRE_ACTIVATE;
	domcall_args.__agency_ctl = agency_ctl;

	domcall_args.slotID = slotID;

	/* Perform a domcall on the specific ME */
	DBG("Pre-activate callback being issued...\n");

	domain_call(domains[slotID], DOMCALL_soo, &domcall_args);

	check_killed_ME();
}

/*
 * soo_cooperate()
 *
 * Perform a cooperate callback in the target (incoming) ME with a set of target ready-to-cooperate MEs.
 *
 */
void soo_cooperate(unsigned int slotID)
{
	soo_domcall_arg_t domcall_args;
	int i, avail_ME;
	bool itself;   /* Used to detect a ME of a same SPID */

	/* Are we OK to collaborate ? */
	if (!domains[slotID]->avz_shared->dom_desc.u.ME.spad.valid)
		return;

	/* Reset anything in the cooperate_args */
	memset(&domcall_args, 0, sizeof(domcall_args));

	domcall_args.cmd = CB_COOPERATE;
	domcall_args.__agency_ctl = agency_ctl;

	domcall_args.u.cooperate_args.role = COOPERATE_INITIATOR;

	avail_ME = 0;

	domcall_args.u.cooperate_args.alone = true;

	for (i = MEMSLOT_BASE; i < MEMSLOT_NR; i++) {

		/* Look for resident ME */
		if ((i != slotID) && memslot[i].busy) {

			/* We are not alone in this smart object */
			domcall_args.u.cooperate_args.alone = false;

			/*
			 * Check if this residing ME has the same SPID than the initiator ME (arrived ME). If the residing ME is not ready
			 * to cooperate, the spid is passed as argument anyway so that the arrived ME can be aware of its presence and decide
			 * to do something accordingly.
			 */
			itself = ((domains[i]->avz_shared->dom_desc.u.ME.spid == domains[slotID]->avz_shared->dom_desc.u.ME.spid) ? true : false);

			/* If the ME authorizes us to enter into a cooperation process... */
			if (domains[i]->avz_shared->dom_desc.u.ME.spad.valid || itself) {

				/* target_coop.slotID contains the slotID of the ME initiator in the cooperation process. */
				domcall_args.u.cooperate_args.u.target_coop.slotID = i;

				domcall_args.u.cooperate_args.u.target_coop.spad = domains[i]->avz_shared->dom_desc.u.ME.spad;

				/* Transfer the SPID of the target ME */
				domcall_args.u.cooperate_args.u.target_coop.spid = domains[i]->avz_shared->dom_desc.u.ME.spid;

				/* Perform a domcall on the specific ME */
				DBG("Cooperate callback being issued...\n");

				domcall_args.slotID = slotID;

				domain_call(domains[slotID], DOMCALL_soo, &domcall_args);
			}
		}
	}

	/* Check if some ME got killed during the cooperation process.
	 * If the initiatore kills itself, the cooperation is done with other residing ME
	 * even if the initiatior is killed.
	 */

	check_killed_ME();
}

static void dump_backtrace(unsigned char key)
{
	soo_domcall_arg_t domcall_args;
	unsigned long flags;

	printk("'%c' pressed -> dumping backtrace in guest\n", key);

	memset(&domcall_args, 0, sizeof(domcall_args));

	domcall_args.cmd = CB_DUMP_BACKTRACE;

	flags = local_irq_save();

	printk("Agency:\n\n");

	domain_call(domains[0], DOMCALL_soo, &domcall_args);

	printk("ME (dom 1):\n\n");

	domain_call(domains[1], DOMCALL_soo, &domcall_args);

	local_irq_restore(flags);
}

static void dump_vbstore(unsigned char key)
{
	soo_domcall_arg_t domcall_args;
	unsigned long flags;

	printk("'%c' pressed -> dumping vbstore (agency) ...\n", key);

	memset(&domcall_args, 0, sizeof(domcall_args));

	domcall_args.cmd = CB_DUMP_VBSTORE;

	flags = local_irq_save();

	domain_call(domains[0], DOMCALL_soo, &domcall_args);

	local_irq_restore(flags);
}


static struct keyhandler dump_backtrace_keyhandler = {
		.fn = dump_backtrace,
		.desc = "dump backtrace"
};

static struct keyhandler dump_vbstore_keyhandler = {
		.fn = dump_vbstore,
		.desc = "dump vbstore"
};

/**
 * Return the descriptor of a domain (agency or ME).
 * A size of 0 means there is no ME in the slot.
 */
void get_dom_desc(unsigned int slotID, dom_desc_t *dom_desc) {
	/* Check for authorization... (to be done) */

	/*
	 * If no ME is present in the slot specified by slotID, we assign a size of 0 in the ME descriptor.
	 * We presume that the slotID of agency is never free...
	 */

	if ((slotID > 1) && !memslot[slotID].busy)
		dom_desc->u.ME.size = 0;
	else
		/* Copy the content to the target desc */
		memcpy(dom_desc, &domains[slotID]->avz_shared->dom_desc, sizeof(dom_desc_t));

}

/**
 * SOO hypercall processing.
 */
void do_soo_hypercall(soo_hyp_t *args) {
	soo_hyp_t op;
	struct domain *dom;
	soo_hyp_dc_event_t *dc_event_args;
	soo_domcall_arg_t domcall_args;
	uint32_t slotID;

	memset(&domcall_args, 0, sizeof(soo_domcall_arg_t));

	/* Get argument from guest */
	memcpy(&op, args, sizeof(soo_hyp_t));

	/*
	 * Execute the hypercall
	 * The usage of args and returns depend on the hypercall itself.
	 * This has to be aligned with the guest which performs the hypercall.
	 */

	switch (op.cmd) {
	case AVZ_MIG_PRE_PROPAGATE:
		soo_pre_propagate(*((unsigned int *) op.p_val1), op.p_val2);
		break;

	case AVZ_MIG_PRE_ACTIVATE:
		soo_pre_activate(*((unsigned int *) op.p_val1));
		break;

	case AVZ_MIG_INIT:
		migration_init(&op);
		break;

	case AVZ_MIG_FINAL:
		migration_final(&op);
		break;

	/* The following hypercall is used during receive operation */
	case AVZ_GET_ME_FREE_SLOT:
	{
		unsigned int ME_size = *((unsigned int *) op.p_val1);
		int slotID;

		/*
		 * Try to get an available slot for a ME with this size.
		 * It will return -1 if no slot is available.
		 */
		slotID = get_ME_free_slot(ME_size, ME_state_migrating);

		*((int *) op.p_val1) = slotID;

		break;
	}

	case AVZ_GET_DOM_DESC:
		get_dom_desc(*((unsigned int *) op.p_val1), (dom_desc_t *) op.p_val2);
		break;

	case AVZ_MIG_READ_MIGRATION_STRUCT:
		read_migration_structures(&op);
		break;

	case AVZ_MIG_WRITE_MIGRATION_STRUCT:
		write_migration_structures(&op);
		break;

	case AVZ_INJECT_ME:
		inject_me(&op);
		break;

	case AVZ_DC_SET:
		/*
		 * AVZ_DC_SET is used to assign a new dc_event number in the (target) domain shared info page.
		 * This has to be done atomically so that if there is still a "pending" value in the field,
		 * the hypercall must return with -BUSY; in this case, the caller has to busy-loop (using schedule preferably)
		 * until the field gets free, i.e. set to DC_NO_EVENT.
		 */
		dc_event_args = (soo_hyp_dc_event_t *) op.p_val1;

		dom = domains[dc_event_args->domID];
		BUG_ON(dom == NULL);

		/* The shared info page is set as non cacheable, i.e. if a CPU tries to update it, it becomes visible to other CPUs */
		if (atomic_cmpxchg(&dom->avz_shared->dc_event, DC_NO_EVENT, dc_event_args->dc_event) != DC_NO_EVENT)
			dc_event_args->state = -EBUSY;
		else
			dc_event_args->state = ESUCCESS;

		break;

	case AVZ_KILL_ME:

		slotID = *((unsigned int *) op.p_val1);
		shutdown_ME(slotID);

		break;

	case AVZ_GET_ME_STATE:

		*((unsigned int *) op.p_val1) = get_ME_state(*((unsigned int *) op.p_val1));

		break;

	case AVZ_SET_ME_STATE:
	{
		unsigned int *state = (unsigned int *) op.p_val1;

		set_ME_state(state[0], state[1]);

		break;
	}

	case AVZ_TRIGGER_LOCAL_COOPERATION:
		slotID = *((unsigned int *) op.p_val1);
		soo_cooperate(slotID);
		break;
		
	case AVZ_AGENCY_CTL:
		/*
		 * Primary agency ctl processing- The args contains the slotID of the ME the agency_ctl is issued from.
		 */

		agency_ctl((agency_ctl_args_t *) op.p_val1);

		break;

	default:
		printk("%s: Unrecognized hypercall: %d\n", __func__, op.cmd);
		BUG();
		break;
	}

	/* If all OK, copy updated structure to guest */
	memcpy(args, &op, sizeof(soo_hyp_t));

	flush_dcache_all();
}

void soo_activity_init(void) {

	DBG("Setting SOO avz up ...\n");

	register_keyhandler('b', &dump_backtrace_keyhandler);
	register_keyhandler('v', &dump_vbstore_keyhandler);
}

