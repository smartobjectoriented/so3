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
#include <ptrace.h>

#include <avz/memslot.h>
#include <avz/domain.h>
#include <avz/sched.h>
#include <avz/injector.h>
#include <avz/evtchn.h>

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
 * @brief  Inject a SO3 container (capsule) as guest domain.
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
 * @param args args received from the guest
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
	strcpy(domctxt->avz_shared.signature, SOO_ME_SIGNATURE);
	
	/* Update the state for the ME instance which will migrate. */
	domctxt->avz_shared.dom_desc.u.ME.state = ME_state_migrating;

	domctxt->pause_count = me->pause_count;

	domctxt->need_periodic_timer = me->need_periodic_timer;

	/* Pause */
	domctxt->pause_flags = me->pause_flags;

	memcpy(&(domctxt->pause_count), &(me->pause_count), sizeof(me->pause_count));

	/* VIRQ mapping */
	memcpy(domctxt->virq_to_evtchn, me->virq_to_evtchn, sizeof((me->virq_to_evtchn)));

	/* 
	 * CPU regs context along the exception path in the hypervisor
	 * right before the context switch. During the context switch,
	 * By far not all registers are preserved.
	 */
        domctxt->vcpu = me->vcpu;

	/* Store the stack frame of this domain */
	memcpy(&domctxt->stack_frame, 
		(void *) (me->domain_stack + DOMAIN_STACK_SIZE - sizeof(struct cpu_regs)),
		sizeof(struct cpu_regs));

        __dump_regs(&domctxt->vcpu.regs);
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

	me->pause_count = domctxt->pause_count;
	me->need_periodic_timer = domctxt->need_periodic_timer;

	/* Pause */
	me->pause_flags = domctxt->pause_flags;

	memcpy(&(me->pause_count), &(domctxt->pause_count), sizeof(me->pause_count));

	/* VIRQ mapping */
	memcpy(me->virq_to_evtchn, domctxt->virq_to_evtchn, sizeof((me->virq_to_evtchn)));

	/* Fields related to CPU */
	me->vcpu = domctxt->vcpu;

        __dump_regs(&me->vcpu.regs);
}

void sync_domain_interactions(unsigned int ME_slotID);
void resume_to_guest(void);

void write_ME_snapshot(avz_hyp_t *args) {
        uint32_t snapshot_size;
        void *snapshot_buffer;
        uint32_t slotID;
        struct domain *domME;
        struct dom_context *domctxt;
	void *dom_stack;
        struct cpu_regs *frame;

        slotID = args->u.avz_snapshot_args.slotID;
	
        snapshot_buffer = (void *) ipa_to_va(MEMSLOT_AGENCY, args->u.avz_snapshot_args.snapshot_paddr);
        snapshot_size = *((uint32_t *) snapshot_buffer);

        printk("## snapshot size: %d\n", snapshot_size);

        domME = domains[slotID];
        domctxt = (struct dom_context *) (snapshot_buffer + sizeof(uint32_t));

        restore_domain_context(args->u.avz_snapshot_args.slotID, domME, domctxt);

	domME->avz_shared->vbstore_grant_ref = args->u.avz_snapshot_args.vbstore_grant_ref;

	/* Copy the ME content */
	memcpy((void *) __xva(slotID, memslot[slotID].base_paddr), snapshot_buffer + sizeof(uint32_t) + sizeof(struct dom_context),
		memslot[slotID].size - sizeof(uint32_t) - sizeof(struct dom_context));
	 
        /* Issue a timer interrupt (first timer IRQ) avoiding some problems during the forced upcall in after_migrate_to_user */
	send_timer_event(domME);

	/* Create a stack devoted to this restored domain */

	dom_stack = memalign(DOMAIN_STACK_SIZE, DOMAIN_STACK_SIZE);
	BUG_ON(!dom_stack);

	/* Keep the reference for fu16ture removal */
        domME->domain_stack = dom_stack;

        /* Reserve the frame which will be restored later */
	frame = dom_stack + DOMAIN_STACK_SIZE - sizeof(cpu_regs_t);

	/* Restore the EL2 frame */
        memcpy(frame, &domctxt->stack_frame, sizeof(struct cpu_regs));

        /* As we will be resumed from the schedule function, we need to update the
         * vital registers from the VCPU regs.
         */
        domME->vcpu.regs.sp = (unsigned long) frame;
	domME->vcpu.regs.lr = (unsigned long) resume_to_guest;


/* TODO: signal behavuiour ...*/
	
	DBG("[soo:avz] %s: Rebinding directcomm event channels: %d (agency) <-> %d (ME)\n", __func__, 
		agency->avz_shared->dom_desc.u.agency.dc_evtchn[slotID], 
		domME->avz_shared->dom_desc.u.ME.dc_evtchn);

	evtchn_bind_existing_interdomain(domME, 
			agency, 
			domME->avz_shared->dom_desc.u.ME.dc_evtchn,
			agency->avz_shared->dom_desc.u.agency.dc_evtchn[slotID]);
					
	DBG("%s: Now, resuming ME slotID %d...\n", __func__, slotID);

	domain_unpause_by_systemcontroller(domME);
}