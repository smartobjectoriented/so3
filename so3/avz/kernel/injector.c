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

#if 1
#define DEBUG
#endif

#include <heap.h>
#include <memory.h>
#include <crc.h>
#include <softirq.h>

#include <avz/memslot.h>
#include <avz/domain.h>
#include <avz/sched.h>
#include <avz/injector.h>

#include <soo/uapi/soo.h>

#include <asm/cacheflush.h>

#include <libfdt/image.h>

#include <libfdt/libfdt.h>

/*
 * Structures to store domain context. Must be here and not locally in function,
 * since the maximum stack size is 8 KB
 */
static struct dom_context domain_context = {0};


/**
 *  Inject a ME within a SOO device. This is the only possibility to load a ME within a Smart Object.
 *
 *  At the entry of this function, the ME ITB could have been allocated in the user space (via the injector application)
 *  or in the vmalloc'd area of the Linux kernel in case of a BT transfer from the tablet (using vuihandler).
 *
 *  To get rid of the way how the page tables are managed by Linux, we perform a copy of the ME ITB in the
 *  AVZ heap, assuming that the 8-MB heap is sufficient to host the ITB ME (< 2 MB in most cases).
 *
 *  If the ITB should become larger, it is still possible to compress (and enhance AVZ with a uncompressor invoked
 *  at loading time). Wouldn't be still not enough, a temporary fixmap mapping combined with get_free_pages should be envisaged
 *  to have the ME ITB accessible from the AVZ user space area.
 *
 * @param op  (op->vaddr is the ITB buffer, op->p_val1 will contain the slodID in return (-1 if no space), op->p_val2 is the ITB buffer size_
 */

void inject_me(avz_hyp_t *args)
{
	int slotID;
	size_t fdt_size;
	void *fdt_vaddr;
        struct domain *domME, *__current;
        unsigned long flags;
        void *itb_vaddr;
        mem_info_t guest_mem_info;

        DBG("%s: Preparing ME injection, source image vaddr = %lx\n", __func__, 
		ipa_to_va(MEMSLOT_AGENCY, args->u.avz_inject_me_args.itb_paddr));

	flags = local_irq_save();

	itb_vaddr = (void *) ipa_to_va(MEMSLOT_AGENCY, args->u.avz_inject_me_args.itb_paddr);

        DBG("%s: ITB vaddr: %lx\n", __func__, itb_vaddr);

	/* Retrieve the domain size of this ME through its device tree. */
	fit_image_get_data_and_size(itb_vaddr, fit_image_get_node(itb_vaddr, "fdt"), (const void **) &fdt_vaddr, &fdt_size);
	if (!fdt_vaddr) {
		printk("### %s: wrong device tree.\n", __func__);
		BUG();
	}

        get_mem_info(fdt_vaddr, &guest_mem_info);

        /* Find a slotID to store this ME. */
        slotID = get_ME_free_slot(guest_mem_info.size, ME_state_booting);
        if (slotID == -1)
		goto out;

        domME = domains[slotID];

	/* Set the size of this ME in its own descriptor with the dom_context size */
	domME->avz_shared->dom_desc.u.ME.size = memslot[slotID].size + sizeof(struct dom_context);

	/* Now set the pfn base of this ME; this will be useful for the Agency Core subsystem */
	domME->avz_shared->dom_desc.u.ME.pfn = phys_to_pfn(memslot[slotID].base_paddr);

	__current = current_domain;

	/* Clear the RAM allocated to this ME */
	memset((void *) __xva(slotID, memslot[slotID].base_paddr), 0, memslot[slotID].size);

	loadME(slotID, itb_vaddr);

	if (construct_ME(domains[slotID]) != 0)
		panic("Could not set up ME guest OS\n");

out:
	/* Prepare to return the slotID to the caller. */
	args->u.avz_inject_me_args.slotID = slotID;

        raise_softirq(SCHEDULE_SOFTIRQ);

        local_irq_restore(flags);
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

	/* Get the start_info structure */
	domctxt->avz_shared = *(me->avz_shared);

	/* Update the state for the ME instance which will migrate. The resident ME keeps its current state. */
	domctxt->avz_shared.dom_desc.u.ME.state = ME_state_migrating;

	domctxt->pause_count = me->pause_count;

	domctxt->need_periodic_timer = me->need_periodic_timer;

	/* Pause */
	domctxt->pause_flags = me->pause_flags;

	memcpy(&(domctxt->pause_count), &(me->pause_count), sizeof(me->pause_count));

	/* VIRQ mapping */
	memcpy(domctxt->virq_to_evtchn, me->virq_to_evtchn, sizeof((me->virq_to_evtchn)));

	/* Arch & address space */
	domctxt->vcpu.regs = me->vcpu.regs;
}

/**
 * Read the ME snapshot.
 */
void read_ME_snapshot(avz_hyp_t *args) {
        unsigned int slotID = args->u.avz_snapshot_args.slotID;
        struct domain *domME = domains[slotID];
        void *snapshot_buffer = (void *) ipa_to_va(MEMSLOT_AGENCY, args->u.avz_snapshot_args.snapshot_paddr);

	/* If the size is 0, we return the snapshot size. */
	if (args->u.avz_snapshot_args.size == 0) {
                args->u.avz_snapshot_args.size = sizeof(uint32_t) + memslot[slotID].size + sizeof(domain_context);
                return;
        }

        /* Gather all the info we need into structures */
        build_domain_context(slotID, domME, &domain_context);

	/* Copy the size of the payload which is made of the dom_info structure and the ME */
        args->u.avz_snapshot_args.size = memslot[slotID].size + sizeof(domain_context);

        memcpy(snapshot_buffer, &args->u.avz_snapshot_args.size, sizeof(uint32_t));
	args->u.avz_snapshot_args.size += sizeof(uint32_t);

	/* Copy the dom_info structure */
        memcpy(snapshot_buffer + sizeof(uint32_t), &domain_context, sizeof(domain_context));

	/* Finally copy the ME */
        memcpy(snapshot_buffer + sizeof(uint32_t) + sizeof(domain_context), (void *) __xva(slotID, memslot[slotID].base_paddr), memslot[slotID].size);
}

/*------------------------------------------------------------------------------
restore_domain_migration_info
restore_vcpu_migration_info
    Restore the migration info in the new ME structure
    Those function are actually exported and called in domain_migrate_restore.c
    They were kept in this file because they are the symmetric functions of
    build_domain_migration_info() and build_vcpu_migration_info()
------------------------------------------------------------------------------*/

void restore_domain_context(unsigned int ME_slotID, struct domain *me, struct dom_context *domctxt)
{
	int i;
	
	DBG("%s\n", __func__);

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

	/* Update the pfn of the ME in its host Smart Object */
	me->avz_shared->dom_desc.u.ME.pfn = phys_to_pfn(memslot[ME_slotID].base_paddr);

	/* start pfn can differ from the initiator according to the physical memory layout */
	me->avz_shared->dom_phys_offset = memslot[ME_slotID].base_paddr;

	me->pause_count = domctxt->pause_count;
	me->need_periodic_timer = domctxt->need_periodic_timer;

	/* Pause */
	me->pause_flags = domctxt->pause_flags;

	memcpy(&(me->pause_count), &(domctxt->pause_count), sizeof(me->pause_count));

	/* VIRQ mapping */
	memcpy(me->virq_to_evtchn, domctxt->virq_to_evtchn, sizeof((me->virq_to_evtchn)));

	/* Fields related to CPU */
	me->vcpu = domctxt->vcpu;
}

void sync_domain_interactions(unsigned int ME_slotID);
void restore_migrated_domain(unsigned int ME_slotID) {
        struct domain *me = NULL;
	addr_t current_pgtable_paddr;

	DBG("Restoring migrated domain on ME_slotID: %d\n", ME_slotID);

	me = domains[ME_slotID];

	restore_domain_context(ME_slotID, me, &domain_context);

	/* Init post-migration execution of ME */

	/* Stack pointer (r13) should remain unchanged since on the receiver side we did not make any push on the SVC stack */
	me->vcpu.regs.sp = (unsigned long) setup_dom_stack(me);

#ifndef CONFIG_ARM64VT
	/* Setting the (future) value of PC in r14 (LR). See code switch_to in entry-armv.S */
	me->vcpu.regs.lr = (unsigned long) (void *) after_migrate_to_user;
#endif

	/* Issue a timer interrupt (first timer IRQ) avoiding some problems during the forced upcall in after_migrate_to_user */
	send_timer_event(me);

	mmu_get_current_pgtable(&current_pgtable_paddr);

	/* Switch to idle domain address space which has a full mapping of the RAM */
	mmu_switch_kernel((void *) idle_domain[smp_processor_id()]->pagetable_paddr);
	/*
	 * Perform synchronization work like memory mappings & vbstore event channel restoration.
	 *
	 * Create the memory mappings in the ME that are normally done at boot
	 * time. We pass the current domain needed by the domcalls to correctly
	 * switch between address spaces */

	DBG("%s: syncing domain interactions in agency...\n", __func__);
	sync_domain_interactions(ME_slotID);

	/* We've done as much initialisation as we could here. */

	ASSERT(smp_processor_id() == 0);

	/*
	 * We check if the ME has been killed during the pre_activate callback.
	 * If yes, we do not pursue our re-activation process.
	 */
	if (get_ME_state(ME_slotID) == ME_state_dead) {

		set_current_domain(agency);
		mmu_switch_kernel((void *) current_pgtable_paddr);

		return ;
	}

	ASSERT(smp_processor_id() == 0);
 
	/* Resume ... */

	/* All sync-ed! Kick the ME alive! */

	ASSERT(smp_processor_id() == 0);

	DBG("%s: Now, resuming ME slotID %d...\n", __func__, ME_slotID);

	domain_unpause_by_systemcontroller(me);

	set_current_domain(agency);
	mmu_switch((void *) current_pgtable_paddr);
}

void write_ME_snapshot(avz_hyp_t *args) {


}