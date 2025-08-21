/*
 * Copyright (C) 2014-2022 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <common.h>
#include <heap.h>
#include <memory.h>
#include <crc.h>
#include <softirq.h>
#include <ptrace.h>

#include <avz/memslot.h>
#include <avz/domain.h>
#include <avz/sched.h>
#include <avz/injector.h>
#include <avz/evtchn.h>
#include <avz/gnttab.h>

#include <soo/uapi/soo.h>

#include <asm/cacheflush.h>
#include <asm/processor.h>

#include <libfdt/image.h>

#include <libfdt/libfdt.h>

/*
 * Structures to store domain context. Must be here and not locally in function,
 * since the maximum stack size is 8 KB
 */
static struct dom_context domain_context = { 0 };

/**
 * @brief  Inject a SO3 container (capsule) as guest domain.
 * 
 * @param args args received from the guest
 */
void inject_capsule(avz_hyp_t *args)
{
	int slotID;
	size_t fdt_size;
	void *fdt_vaddr;
	struct domain *domME, *__current;
	void *itb_vaddr;
	mem_info_t guest_mem_info;

	LOG_DEBUG("%s: Preparing ME injection, source image vaddr = %lx\n", __func__,
		  ipa_to_va(MEMSLOT_AGENCY, args->u.avz_inject_capsule_args.itb_paddr));

	BUG_ON(local_irq_is_enabled());

	itb_vaddr = (void *) ipa_to_va(MEMSLOT_AGENCY, args->u.avz_inject_capsule_args.itb_paddr);

	LOG_DEBUG("%s: ITB vaddr: %lx\n", __func__, itb_vaddr);

	/* Retrieve the domain size of this ME through its device tree. */
	fit_image_get_data_and_size(itb_vaddr, fit_image_get_node(itb_vaddr, "fdt"), (const void **) &fdt_vaddr, &fdt_size);
	if (!fdt_vaddr) {
		printk("### %s: wrong device tree.\n", __func__);
		BUG();
	}

	get_mem_info(fdt_vaddr, &guest_mem_info);

	/* Find a slotID to store this capsule */
	slotID = get_ME_free_slot(guest_mem_info.size, args->u.avz_inject_capsule_args.slotID);
	if (slotID == -1)
		goto out;

	domME = domains[slotID];

	/* At the beginning, the capsule is stopped */
	domME->avz_shared->dom_desc.u.ME.state = ME_state_stopped;

	/* Set the size of this ME in its own descriptor with the dom_context size */
	domME->avz_shared->dom_desc.u.ME.size = memslot[slotID].size;

	__current = current_domain;

	/* Clear the RAM allocated to this capsule */
	memset((void *) __xva(slotID, memslot[slotID].base_paddr), 0, memslot[slotID].size);

	loadME(slotID, itb_vaddr);

	if (construct_ME(domains[slotID]) != 0)
		panic("Could not set up ME guest OS\n");

out:
	/* Prepare to return the slotID to the caller. */
	args->u.avz_inject_capsule_args.slotID = slotID;

	domME->avz_shared->dom_desc.u.ME.vbstore_pfn = map_vbstore_pfn(slotID, 0);
	domME->avz_shared->dom_desc.u.ME.vbstore_revtchn = agency->avz_shared->dom_desc.u.agency.vbstore_evtchn[slotID];
}

/**
 * @brief Start the execution of a pre-loaded capsule
 * 
 * @param args 
 */
void start_capsule(avz_hyp_t *args)
{
	BUG_ON(local_irq_is_enabled());

	raise_softirq(SCHEDULE_SOFTIRQ);

	domain_unpause_by_systemcontroller(domains[args->u.avz_start_capsule_args.slotID]);

	/* Setting the capsule in living will be made by Linux since there are still
	 * FE/BE to be resumed.
	 */
}

/*------------------------------------------------------------------------------
build_domain_migration_info
build_vcpu_migration_info
    Build the structures holding the key info to be migrated over
------------------------------------------------------------------------------*/
static void build_domain_context(unsigned int ME_slotID, struct domain *me, struct dom_context *domctxt)
{
	/* Event channel info */
	memcpy(domctxt->evtchn, me->evtchn, sizeof(me->evtchn));

	/* Preserve the current system time to facilitate resuming after hibernation */
	me->avz_shared->current_s_time = NOW();

	/* Get the start_info structure */
	domctxt->avz_shared = *(me->avz_shared);
	strcpy(domctxt->avz_shared.signature, SOO_ME_SIGNATURE);

	/* The snapshot will contain a capsule with the state ME_state_suspended only
	 * if the capsule was living, otherwise it has to be in ME_state_stopped, right
	 * before its execution. 
	*/
	if (me->avz_shared->dom_desc.u.ME.state == ME_state_suspended)
		domctxt->avz_shared.dom_desc.u.ME.state = ME_state_hibernate;

	BUG_ON((me->avz_shared->dom_desc.u.ME.state != ME_state_stopped) &&
	       (me->avz_shared->dom_desc.u.ME.state != ME_state_suspended));

	domctxt->pause_count = me->pause_count;

	domctxt->need_periodic_timer = me->need_periodic_timer;

	/* Pause */
	domctxt->pause_flags = me->pause_flags;

	memcpy(&domctxt->grant_pfn, &me->grant_pfn, sizeof(me->grant_pfn));

	memcpy(&(domctxt->pause_count), &(me->pause_count), sizeof(me->pause_count));

	/* VIRQ mapping */
	memcpy(domctxt->virq_to_evtchn, me->virq_to_evtchn, sizeof(me->virq_to_evtchn));

	/* Store the IPA physical address base */
	domctxt->ipa_addr = memslot[ME_slotID].ipa_addr;

	/*
         * CPU regs context along the exception path in the hypervisor
         * right before the context switch. During the context switch,
         * By far not all registers are preserved.
         */
	domctxt->vcpu = me->vcpu;

	/* Store the stack frame of this domain */
	memcpy(&domctxt->stack_frame, (void *) (me->domain_stack + DOMAIN_STACK_SIZE - sizeof(struct cpu_regs)),
	       sizeof(struct cpu_regs));
}

/**
  * @brief Take a memory snapshot of a capsule. This will lead to a capsule with HIBERNATE state
  * 	   while the resident capsule will end up in resuming state.
  * 
  * @param args provided from the Linux kernel
  */
void read_ME_snapshot(avz_hyp_t *args)
{
	unsigned int slotID = args->u.avz_snapshot_args.slotID;
	struct domain *domME = domains[slotID];
	void *snapshot_buffer = (void *) ipa_to_va(MEMSLOT_AGENCY, args->u.avz_snapshot_args.snapshot_paddr);

	/* If the size is 0, we return the snapshot size. */
	if (args->u.avz_snapshot_args.size == 0) {
		args->u.avz_snapshot_args.size = sizeof(uint32_t) + memslot[slotID].size + sizeof(domain_context);
		return;
	}

	/* If the capsule is living, it will be put in ME_state_suspended state by Linux
	 * before being entering this function.  
	*/
	if (domME->avz_shared->dom_desc.u.ME.state == ME_state_suspended) {
		/* Pause the capsule */
		domain_pause_by_systemcontroller(domME);
	}

	/* Gather all the info we need into structures */
	/* This will put the capsule snapshot in HIBERNATE state */
	build_domain_context(slotID, domME, &domain_context);

	/* Copy the size of the payload which is made of the dom_info structure and the capsule */
	args->u.avz_snapshot_args.size = memslot[slotID].size + sizeof(domain_context);

	memcpy(snapshot_buffer, &args->u.avz_snapshot_args.size, sizeof(uint32_t));
	args->u.avz_snapshot_args.size += sizeof(uint32_t);

	/* Copy the dom_info structure */
	memcpy(snapshot_buffer + sizeof(uint32_t), &domain_context, sizeof(domain_context));

	/* Finally copy the ME */
	memcpy(snapshot_buffer + sizeof(uint32_t) + sizeof(domain_context), (void *) __xva(slotID, memslot[slotID].base_paddr),
	       memslot[slotID].size);

	if (domME->avz_shared->dom_desc.u.ME.state == ME_state_suspended) {
		/* Now, this ME is suspended and must be resumed by the agency */
		domME->avz_shared->dom_desc.u.ME.state = ME_state_resuming;

		domain_unpause_by_systemcontroller(domME);
	}
}

/**
 * @brief Recover the dom_context structure from a pre-saved capsule
 * 
 * @param ME_slotID 
 * @param me 
 * @param domctxt 
 */
void restore_domain_context(unsigned int ME_slotID, struct domain *me, struct dom_context *domctxt)
{
	int i;

	LOG_DEBUG("%s\n", __func__);

	*(me->avz_shared) = domctxt->avz_shared;

	/* Check that our signature is valid so that the image transfer should be good. */
	if (strcmp(me->avz_shared->signature, SOO_ME_SIGNATURE))
		panic("%s: Cannot find the correct signature in the shared page (" SOO_ME_SIGNATURE ")...\n", __func__);

	/* Update the domID of course */
	me->avz_shared->domID = ME_slotID;

	memcpy(me->evtchn, domctxt->evtchn, sizeof(me->evtchn));

	/*
	 * We reconfigure the inter-domain event channel so that we unbind the link to the previous
	 * remote domain (the agency in most cases), but we keep the state as it is since we do not
	 * want that the local event channel gets changed.
	 *
	 * Re-binding is performed during the resuming via vbus (backend side) OR
	 * if the ME gets killed, the event channel will be closed without any effect to a remote domain.
	 */

	for (i = 0; i < NR_EVTCHN; i++)
		if (me->evtchn[i].state == ECS_INTERDOMAIN)
			me->evtchn[i].interdomain.remote_dom = NULL;

	me->pause_count = domctxt->pause_count;
	me->need_periodic_timer = domctxt->need_periodic_timer;

	/* Pause */
	me->pause_flags = domctxt->pause_flags;

	memcpy(&me->grant_pfn, &domctxt->grant_pfn, sizeof(me->grant_pfn));

	memcpy(&(me->pause_count), &(domctxt->pause_count), sizeof(me->pause_count));

	/* VIRQ mapping */
	memcpy(me->virq_to_evtchn, domctxt->virq_to_evtchn, sizeof((me->virq_to_evtchn)));

	/* IPA physical address base */
	memslot[ME_slotID].ipa_addr = domctxt->ipa_addr;

	/* Fields related to CPU */
	me->vcpu = domctxt->vcpu;
}
/**
 * @brief Write a snapshot into the memory. 
 * 
 * @param args If args->u.avz_snapshot_args.size == 0, the function will try to find an empty slot.
 */
void write_ME_snapshot(avz_hyp_t *args)
{
	uint32_t snapshot_size;
	void *snapshot_buffer;
	uint32_t slotID;
	struct domain *domME;
	struct dom_context *domctxt;
	void *dom_stack;
	struct cpu_regs *frame;

	slotID = args->u.avz_snapshot_args.slotID;
	snapshot_size = args->u.avz_snapshot_args.size;

	/* Ask for available slot and perform the reservation */

	LOG_DEBUG("Original size of the snapshot: %d bytes\n", snapshot_size);
	LOG_DEBUG("Looking for an available slot for a capsule of %d bytes...\n",
		  snapshot_size - sizeof(uint32_t) - sizeof(struct dom_context));

	slotID = get_ME_free_slot(snapshot_size - sizeof(uint32_t) - sizeof(struct dom_context), slotID);
	if (slotID > 0)
		args->u.avz_snapshot_args.slotID = slotID;
	else
		return;

	LOG_DEBUG("Available slotID: %d\n", args->u.avz_snapshot_args.slotID);

	LOG_DEBUG("Writing the snapshot into memory...\n");
	snapshot_buffer = (void *) ipa_to_va(MEMSLOT_AGENCY, args->u.avz_snapshot_args.snapshot_paddr);

	domME = domains[slotID];
	domctxt = (struct dom_context *) (snapshot_buffer + sizeof(uint32_t));

	LOG_DEBUG("Restoring the domain context...\n");
	restore_domain_context(slotID, domME, domctxt);

	LOG_DEBUG("Set up the page tables...\n");
	__setup_dom_pgtable(domME, memslot[slotID].base_paddr, memslot[slotID].size);

	/* Copy the ME content */
	memcpy((void *) __xva(slotID, memslot[slotID].base_paddr),
	       snapshot_buffer + sizeof(uint32_t) + sizeof(struct dom_context), memslot[slotID].size);

	/* Create a stack for this restored domain */

	dom_stack = memalign(DOMAIN_STACK_SIZE, DOMAIN_STACK_SIZE);
	BUG_ON(!dom_stack);

	/* Keep the reference for future removal */
	domME->domain_stack = dom_stack;

	/* Reserve the frame which will be restored later */
	frame = dom_stack + DOMAIN_STACK_SIZE - sizeof(cpu_regs_t);

	/* Restore the EL2 frame */
	memcpy(frame, &domctxt->stack_frame, sizeof(struct cpu_regs));

	/* We need to re-map the vbstore page corresponding to this slotID */
	map_vbstore_pfn(domME->avz_shared->domID, domME->avz_shared->dom_desc.u.ME.vbstore_pfn);
	LOG_DEBUG("State of the saved capsule: %d\n", domME->avz_shared->dom_desc.u.ME.state);

	if (domME->avz_shared->dom_desc.u.ME.state != ME_state_stopped) {
		BUG_ON(domME->avz_shared->dom_desc.u.ME.state != ME_state_hibernate);

		/* As we will be resumed from the schedule function, we need to update the
		 * CPU registers from the VCPU regs.
		 */
		domME->vcpu.regs.sp = (unsigned long) frame;
		domME->vcpu.regs.x21 = (unsigned long) domME->avz_shared->dom_desc.u.ME.resume_fn;

		domME->vcpu.regs.lr = (unsigned long) resume_to_guest;

		/* Now restoring event channel configuration */
		evtchn_bind_existing_interdomain(domME, agency, domME->avz_shared->dom_desc.u.ME.vbstore_levtchn,
						 agency->avz_shared->dom_desc.u.agency.vbstore_evtchn[slotID]);

		LOG_DEBUG("%s: Rebinding directcomm event channels: %d (agency) <-> %d (ME)\n", __func__,
			  agency->avz_shared->dom_desc.u.agency.dc_evtchn[slotID], domME->avz_shared->dom_desc.u.ME.dc_evtchn);

		evtchn_bind_existing_interdomain(domME, agency, domME->avz_shared->dom_desc.u.ME.dc_evtchn,
						 agency->avz_shared->dom_desc.u.agency.dc_evtchn[slotID]);
	}
	LOG_DEBUG("%s: Now, resuming ME slotID %d...\n", __func__, slotID);

	domain_unpause_by_systemcontroller(domME);
}

void __sigreturn(void)
{
	current_domain->avz_shared->dom_desc.u.ME.state = ME_state_awakened;

	send_timer_event(current_domain);
}
