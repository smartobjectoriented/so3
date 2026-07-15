/*
 * Copyright (C) 2014-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <log.h>

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

/* Maximum amount of domain memory moved in one AVZ_STAGE_CHUNK call.
 * This bounds the time spent at EL2 with IRQs off on the calling CPU
 * (issue #287).
 */
#define STAGE_CHUNK_SIZE (4 * SZ_1M)

/**
 * Compute the size of the next chunk to be moved, and advance the cursor.
 */
static size_t stage_next_chunk(uint32_t *offset, size_t total)
{
	size_t chunk_size = total - *offset;

	if (chunk_size > STAGE_CHUNK_SIZE)
		chunk_size = STAGE_CHUNK_SIZE;

	*offset += chunk_size;

	return chunk_size;
}

/**
 * Check that a slot currently holds a capsule, i.e. it is allocated and its
 * domain exists. @state, if not S3C_state_dead, is additionally required.
 */
static bool staged_slot_valid(int slotID, S3C_state_t state)
{
	if ((slotID < MEMSLOT_BASE) || (slotID >= MEMSLOT_NR) || !memslot[slotID].busy || (domains[slotID] == NULL))
		return false;

	return (state == S3C_state_dead) || (domains[slotID]->avz_shared->dom_desc.u.S3C.state == state);
}

/**
 * @brief  Inject a SO3 container (capsule) as guest domain.
 *
 * The injection is staged (issue #287): INIT parses the capsule ITB and
 * allocates the memslot, CHUNK wipes the slot RAM chunk by chunk and
 * FINALIZE loads the capsule image and constructs the domain. The agency
 * drives the sequence with one hypercall per stage so that the calling CPU
 * gets its interrupts back between two stages instead of staying at EL2
 * with IRQs off for the whole (large) slot processing.
 *
 * @param args args received from the guest
 */
void inject_capsule(avz_hyp_t *args)
{
	int slotID;
	size_t fdt_size, chunk_size;
	uint32_t offset;
	void *fdt_vaddr;
	struct domain *dom_S3C;
	void *itb_vaddr;
	mem_info_t guest_mem_info;

	BUG_ON(local_irq_is_enabled());

	itb_vaddr = (void *) ipa_to_va(MEMSLOT_AGENCY, args->u.avz_inject_capsule_args.itb_paddr);

	switch (args->u.avz_inject_capsule_args.stage) {
	case AVZ_STAGE_INIT:

		LOG_DEBUG("%s: Preparing capsule injection, source image vaddr = %lx\n", __func__, itb_vaddr);

		/* Retrieve the domain size of this capsule through its device tree. */
		fit_image_get_data_and_size(itb_vaddr, fit_image_get_node(itb_vaddr, "fdt"), (const void **) &fdt_vaddr,
					    &fdt_size);
		if (!fdt_vaddr) {
			printk("### %s: wrong device tree.\n", __func__);
			BUG();
		}

		get_mem_info(fdt_vaddr, &guest_mem_info);

		/* Find a slotID to store this capsule */
		slotID = get_S3C_free_slot(guest_mem_info.size, args->u.avz_inject_capsule_args.slotID);
		if (slotID == -1) {
			printk("%s: no slot available for a capsule of %d bytes.\n", __func__, guest_mem_info.size);
			args->u.avz_inject_capsule_args.slotID = -1;
			return;
		}

		dom_S3C = domains[slotID];

		/* At the beginning, the capsule is stopped */
		dom_S3C->avz_shared->dom_desc.u.S3C.state = S3C_state_stopped;

		/* Store slotID & capsuleID */
		dom_S3C->avz_shared->dom_desc.u.S3C.slotID = slotID;
		dom_S3C->avz_shared->dom_desc.u.S3C.capsuleID = args->u.avz_inject_capsule_args.capsuleID;

		/* Set the size of this capsule in its own descriptor with the dom_context size */
		dom_S3C->avz_shared->dom_desc.u.S3C.size = memslot[slotID].size;

		/* Return the slotID and the amount of slot memory to be cleared. */

		args->u.avz_inject_capsule_args.slotID = slotID;
		args->u.avz_inject_capsule_args.offset = memslot[slotID].size;

		break;

	case AVZ_STAGE_CHUNK:

		slotID = args->u.avz_inject_capsule_args.slotID;
		offset = args->u.avz_inject_capsule_args.offset;

		if (!staged_slot_valid(slotID, S3C_state_stopped) || (offset >= memslot[slotID].size)) {
			printk("%s: invalid CHUNK stage (slot %d, offset 0x%x)\n", __func__, slotID, offset);
			args->u.avz_inject_capsule_args.slotID = -1;
			return;
		}

		/* Clear the next chunk of the RAM allocated to this capsule */

		chunk_size = stage_next_chunk(&args->u.avz_inject_capsule_args.offset, memslot[slotID].size);

		memset((void *) __xva(slotID, memslot[slotID].base_paddr + offset), 0, chunk_size);

		break;

	case AVZ_STAGE_FINALIZE:

		slotID = args->u.avz_inject_capsule_args.slotID;

		if (!staged_slot_valid(slotID, S3C_state_stopped)) {
			printk("%s: invalid FINALIZE stage (slot %d)\n", __func__, slotID);
			args->u.avz_inject_capsule_args.slotID = -1;
			return;
		}

		dom_S3C = domains[slotID];

		load_S3C(slotID, itb_vaddr);

		if (construct_S3C(domains[slotID]) != 0)
			panic("Could not set up capsule guest OS\n");

		dom_S3C->avz_shared->dom_desc.u.S3C.vbstore_pfn = map_vbstore_pfn(slotID, 0);
		dom_S3C->avz_shared->dom_desc.u.S3C.vbstore_revtchn =
			agency->avz_shared->dom_desc.u.agency.vbstore_evtchn[slotID];

		break;

	default:
		printk("%s: unknown injection stage %d\n", __func__, args->u.avz_inject_capsule_args.stage);
		args->u.avz_inject_capsule_args.slotID = -1;
		break;
	}
}

/**
 * @brief Start the execution of a pre-loaded capsule
 * 
 * @param args 
 */
void start_capsule(avz_hyp_t *args)
{
	unsigned int slotID = args->u.avz_start_capsule_args.slotID;

	BUG_ON(local_irq_is_enabled());

	if ((slotID >= MAX_DOMAINS) || (domains[slotID] == NULL)) {
		printk("%s: no capsule in slot %d, ignoring.\n", __func__, slotID);
		return;
	}

	raise_softirq(SCHEDULE_SOFTIRQ);

	domain_unpause_by_systemcontroller(domains[slotID]);

	/* Setting the capsule in living will be made by Linux since there are still
	 * FE/BE to be resumed.
	 */
}

/*------------------------------------------------------------------------------
build_domain_context
    Build the structures holding the key domain info for the snapshot.
------------------------------------------------------------------------------*/
static void build_domain_context(unsigned int S3C_slotID, struct domain *me, struct dom_context *domctxt)
{
	/* Event channel info */
	memcpy(domctxt->evtchn, me->evtchn, sizeof(me->evtchn));

	/* current_s_time is written by the capsule itself (in ITS OWN time
	 * scale) when it handles DC_SUSPEND — do not overwrite it here with
	 * AVZ's EL2 time, the two time bases are unrelated and the resume
	 * offset computed from a mixed pair wraps the timer deadlines. */

	/* Get the start_info structure */
	domctxt->avz_shared = *(me->avz_shared);
	strcpy(domctxt->avz_shared.signature, SOO_S3C_SIGNATURE);

	/* The snapshot will contain a capsule with the state S3C_state_suspended only
	 * if the capsule was living, otherwise it has to be in S3C_state_stopped, right
	 * before its execution. 
	*/
	if (me->avz_shared->dom_desc.u.S3C.state == S3C_state_suspended)
		domctxt->avz_shared.dom_desc.u.S3C.state = S3C_state_hibernate;

	BUG_ON((me->avz_shared->dom_desc.u.S3C.state != S3C_state_stopped) &&
	       (me->avz_shared->dom_desc.u.S3C.state != S3C_state_suspended));

	domctxt->pause_count = me->pause_count;

	domctxt->need_periodic_timer = me->need_periodic_timer;

	/* Pause */
	domctxt->pause_flags = me->pause_flags;

	memcpy(&domctxt->grant_pfn, &me->grant_pfn, sizeof(me->grant_pfn));
	memcpy(&domctxt->fbdev_start_pfn, &me->fbdev_start_pfn, sizeof(me->fbdev_start_pfn));

	memcpy(&(domctxt->pause_count), &(me->pause_count), sizeof(me->pause_count));

	/* VIRQ mapping */
	memcpy(domctxt->virq_to_evtchn, me->virq_to_evtchn, sizeof(me->virq_to_evtchn));

	/* Store the IPA physical address base */
	domctxt->ipa_addr = memslot[S3C_slotID].ipa_addr;

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
  * Staged like the injection (issue #287): INIT pauses the capsule and writes
  * the snapshot header (payload size + domain context), CHUNK copies the
  * capsule memory chunk by chunk and FINALIZE resumes the capsule. A size of
  * 0 at the INIT stage only queries the snapshot size, as before.
  *
  * @param args provided from the Linux kernel
  */
void read_S3C_snapshot(avz_hyp_t *args)
{
	unsigned int slotID = args->u.avz_snapshot_args.slotID;
	size_t chunk_size, payload_offset;
	uint32_t offset;
	struct domain *dom_S3C;
	void *snapshot_buffer = (void *) ipa_to_va(MEMSLOT_AGENCY, args->u.avz_snapshot_args.snapshot_paddr);

	BUG_ON(local_irq_is_enabled());

	if (!staged_slot_valid(slotID, S3C_state_dead)) {
		printk("%s: no capsule in slot %d, ignoring.\n", __func__, slotID);
		args->u.avz_snapshot_args.size = 0;
		args->u.avz_snapshot_args.offset = 0;
		return;
	}

	dom_S3C = domains[slotID];

	/* The capsule memory sits after the header (payload size + context). */

	payload_offset = sizeof(uint32_t) + sizeof(domain_context);

	switch (args->u.avz_snapshot_args.stage) {
	case AVZ_STAGE_INIT:

		/* If the size is 0, we return the snapshot size. */
		if (args->u.avz_snapshot_args.size == 0) {
			args->u.avz_snapshot_args.size = payload_offset + memslot[slotID].size;
			args->u.avz_snapshot_args.offset = memslot[slotID].size;
			return;
		}

		/* If the capsule is living, it will be put in S3C_state_suspended state by Linux
		 * before being entering this function.
		*/
		if (dom_S3C->avz_shared->dom_desc.u.S3C.state == S3C_state_suspended) {
			/* Pause the capsule */
			domain_pause_by_systemcontroller(dom_S3C);
		}

		/* Gather all the info we need into structures */
		/* This will put the capsule snapshot in HIBERNATE state */
		build_domain_context(slotID, dom_S3C, &domain_context);

		/* Copy the size of the payload which is made of the dom_info structure and the capsule */
		args->u.avz_snapshot_args.size = memslot[slotID].size + sizeof(domain_context);

		memcpy(snapshot_buffer, &args->u.avz_snapshot_args.size, sizeof(uint32_t));
		args->u.avz_snapshot_args.size += sizeof(uint32_t);

		/* Copy the dom_info structure */
		memcpy(snapshot_buffer + sizeof(uint32_t), &domain_context, sizeof(domain_context));

		/* The capsule memory itself is copied by the CHUNK stage. */

		args->u.avz_snapshot_args.offset = memslot[slotID].size;

		break;

	case AVZ_STAGE_CHUNK:

		offset = args->u.avz_snapshot_args.offset;

		if (offset >= memslot[slotID].size) {
			printk("%s: invalid CHUNK stage (slot %d, offset 0x%x)\n", __func__, slotID, offset);
			args->u.avz_snapshot_args.size = 0;
			return;
		}

		chunk_size = stage_next_chunk(&args->u.avz_snapshot_args.offset, memslot[slotID].size);

		memcpy(snapshot_buffer + payload_offset + offset, (void *) __xva(slotID, memslot[slotID].base_paddr + offset),
		       chunk_size);

		break;

	case AVZ_STAGE_FINALIZE:

		if (dom_S3C->avz_shared->dom_desc.u.S3C.state == S3C_state_suspended) {
			/* Now, this capsule is suspended and must be resumed by the agency */
			dom_S3C->avz_shared->dom_desc.u.S3C.state = S3C_state_resuming;

			domain_unpause_by_systemcontroller(dom_S3C);
		}

		break;

	default:
		printk("%s: unknown snapshot stage %d\n", __func__, args->u.avz_snapshot_args.stage);
		args->u.avz_snapshot_args.size = 0;
		break;
	}
}

/**
 * @brief Recover the dom_context structure from a pre-saved capsule
 * 
 * @param S3C_slotID 
 * @param me 
 * @param domctxt 
 */
void restore_domain_context(unsigned int S3C_slotID, struct domain *me, struct dom_context *domctxt)
{
	int i;

	LOG_DEBUG("%s\n", __func__);

	*(me->avz_shared) = domctxt->avz_shared;

	/* Check that our signature is valid so that the image transfer should be good. */
	if (strcmp(me->avz_shared->signature, SOO_S3C_SIGNATURE))
		panic("%s: Cannot find the correct signature in the shared page (" SOO_S3C_SIGNATURE ")...\n", __func__);

	/* Update the domID of course */
	me->avz_shared->domID = S3C_slotID;

	memcpy(me->evtchn, domctxt->evtchn, sizeof(me->evtchn));

	/*
	 * We reconfigure the inter-domain event channel so that we unbind the link to the previous
	 * remote domain (the agency in most cases), but we keep the state as it is since we do not
	 * want that the local event channel gets changed.
	 *
	 * Re-binding is performed during the resuming via vbus (backend side) OR
	 * if the capsule gets killed, the event channel will be closed without any effect to a remote domain.
	 */

	for (i = 0; i < NR_EVTCHN; i++)
		if (me->evtchn[i].state == ECS_INTERDOMAIN)
			me->evtchn[i].interdomain.remote_dom = NULL;

	me->pause_count = domctxt->pause_count;
	me->need_periodic_timer = domctxt->need_periodic_timer;

	/* Pause */
	me->pause_flags = domctxt->pause_flags;

	memcpy(&me->grant_pfn, &domctxt->grant_pfn, sizeof(me->grant_pfn));
	memcpy(&me->fbdev_start_pfn, &domctxt->fbdev_start_pfn, sizeof(me->fbdev_start_pfn));

	memcpy(&(me->pause_count), &(domctxt->pause_count), sizeof(me->pause_count));

	/* VIRQ mapping */
	memcpy(me->virq_to_evtchn, domctxt->virq_to_evtchn, sizeof((me->virq_to_evtchn)));

	/* IPA physical address base */
	memslot[S3C_slotID].ipa_addr = domctxt->ipa_addr;

	/* Fields related to CPU */
	me->vcpu = domctxt->vcpu;
}
/**
 * @brief Write a snapshot into the memory.
 *
 * Staged like the injection (issue #287): INIT allocates the slot, restores
 * the domain context and sets up the page tables, CHUNK copies the capsule
 * memory chunk by chunk and FINALIZE builds the stack, rebinds the event
 * channels and resumes the capsule.
 *
 * @param args If args->u.avz_snapshot_args.size == 0, the function will try to find an empty slot.
 */
void write_S3C_snapshot(avz_hyp_t *args)
{
	uint32_t snapshot_size;
	void *snapshot_buffer;
	uint32_t slotID;
	size_t chunk_size, payload_offset;
	uint32_t offset;
	struct domain *dom_S3C;
	struct dom_context *domctxt;
	void *dom_stack;
	struct cpu_regs *frame;

	BUG_ON(local_irq_is_enabled());

	slotID = args->u.avz_snapshot_args.slotID;
	snapshot_size = args->u.avz_snapshot_args.size;
	snapshot_buffer = (void *) ipa_to_va(MEMSLOT_AGENCY, args->u.avz_snapshot_args.snapshot_paddr);

	/* The capsule memory sits after the header (payload size + context). */

	payload_offset = sizeof(uint32_t) + sizeof(struct dom_context);
	domctxt = (struct dom_context *) (snapshot_buffer + sizeof(uint32_t));

	if (args->u.avz_snapshot_args.stage == AVZ_STAGE_INIT) {
		/* Ask for available slot and perform the reservation */

		LOG_DEBUG("Original size of the snapshot: %d bytes\n", snapshot_size);
		LOG_DEBUG("Looking for an available slot for a capsule of %d bytes...\n", snapshot_size - payload_offset);

		slotID = get_S3C_free_slot(snapshot_size - payload_offset, slotID);
		if (slotID > 0)
			args->u.avz_snapshot_args.slotID = slotID;
		else {
			printk("%s: no slot available for a snapshot of %d bytes.\n", __func__, snapshot_size - payload_offset);
			args->u.avz_snapshot_args.slotID = -1;
			return;
		}

		LOG_DEBUG("Available slotID: %d\n", args->u.avz_snapshot_args.slotID);

		LOG_DEBUG("Writing the snapshot into memory...\n");

		dom_S3C = domains[slotID];

		LOG_DEBUG("Restoring the domain context...\n");
		restore_domain_context(slotID, dom_S3C, domctxt);

		LOG_DEBUG("Set up the page tables...\n");
		__setup_dom_pgtable(dom_S3C, memslot[slotID].base_paddr, memslot[slotID].size);

		/* The capsule memory itself is copied by the CHUNK stage. */

		args->u.avz_snapshot_args.offset = memslot[slotID].size;

		return;
	}

	if (!staged_slot_valid(slotID, S3C_state_dead)) {
		printk("%s: invalid stage %d on slot %d\n", __func__, args->u.avz_snapshot_args.stage, slotID);
		args->u.avz_snapshot_args.slotID = -1;
		return;
	}

	dom_S3C = domains[slotID];

	if (args->u.avz_snapshot_args.stage == AVZ_STAGE_CHUNK) {
		offset = args->u.avz_snapshot_args.offset;

		if (offset >= memslot[slotID].size) {
			printk("%s: invalid CHUNK stage (slot %d, offset 0x%x)\n", __func__, slotID, offset);
			args->u.avz_snapshot_args.slotID = -1;
			return;
		}

		/* Copy the next chunk of the capsule content */

		chunk_size = stage_next_chunk(&args->u.avz_snapshot_args.offset, memslot[slotID].size);

		memcpy((void *) __xva(slotID, memslot[slotID].base_paddr + offset), snapshot_buffer + payload_offset + offset,
		       chunk_size);

		return;
	}

	if (args->u.avz_snapshot_args.stage != AVZ_STAGE_FINALIZE) {
		printk("%s: unknown snapshot stage %d\n", __func__, args->u.avz_snapshot_args.stage);
		args->u.avz_snapshot_args.slotID = -1;
		return;
	}

	/* Create a stack for this restored domain */

	dom_stack = memalign(DOMAIN_STACK_SIZE, DOMAIN_STACK_SIZE);
	BUG_ON(!dom_stack);

	/* Keep the reference for future removal */
	dom_S3C->domain_stack = dom_stack;

	/* Reserve the frame which will be restored later */
	frame = dom_stack + DOMAIN_STACK_SIZE - sizeof(cpu_regs_t);

	/* Restore the EL2 frame */
	memcpy(frame, &domctxt->stack_frame, sizeof(struct cpu_regs));

	/* We need to re-map the vbstore page corresponding to this slotID */
	map_vbstore_pfn(dom_S3C->avz_shared->domID, dom_S3C->avz_shared->dom_desc.u.S3C.vbstore_pfn);
	LOG_DEBUG("State of the saved capsule: %d\n", dom_S3C->avz_shared->dom_desc.u.S3C.state);

	if (dom_S3C->avz_shared->dom_desc.u.S3C.state != S3C_state_stopped) {
		BUG_ON(dom_S3C->avz_shared->dom_desc.u.S3C.state != S3C_state_hibernate);

		/* As we will be resumed from the schedule function, we need to update the
		 * CPU registers from the VCPU regs.
		 */
		dom_S3C->vcpu.regs.sp = (unsigned long) frame;
		dom_S3C->vcpu.regs.x21 = (unsigned long) dom_S3C->avz_shared->dom_desc.u.S3C.resume_fn;

		dom_S3C->vcpu.regs.lr = (unsigned long) resume_to_guest;

		/* Now restoring event channel configuration */
		evtchn_bind_existing_interdomain(dom_S3C, agency, dom_S3C->avz_shared->dom_desc.u.S3C.vbstore_levtchn,
						 agency->avz_shared->dom_desc.u.agency.vbstore_evtchn[slotID]);

		LOG_DEBUG("%s: Rebinding directcomm event channels: %d (agency) <-> %d (capsule)\n", __func__,
			  agency->avz_shared->dom_desc.u.agency.dc_evtchn[slotID],
			  dom_S3C->avz_shared->dom_desc.u.S3C.dc_evtchn);

		evtchn_bind_existing_interdomain(dom_S3C, agency, dom_S3C->avz_shared->dom_desc.u.S3C.dc_evtchn,
						 agency->avz_shared->dom_desc.u.agency.dc_evtchn[slotID]);
	}
	LOG_DEBUG("%s: Now, resuming capsule slotID %d...\n", __func__, slotID);

	domain_unpause_by_systemcontroller(dom_S3C);
}

void __sigreturn(void)
{
	current_domain->avz_shared->dom_desc.u.S3C.state = S3C_state_awakened;

	send_timer_event(current_domain);
}
