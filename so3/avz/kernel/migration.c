
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

#include <smp.h>
#include <types.h>
#include <console.h>
#include <heap.h>
#include <crc.h>

#include <asm/cacheflush.h>
#include <asm/migration.h>

#include <device/irq.h>

#include <avz/memslot.h>
#include <avz/migration.h>
#include <avz/domain.h>
#include <avz/debug.h>
#include <avz/soo.h>

#include <libfdt/image.h>
#include <libfdt/libfdt.h>

#include <avz/uapi/avz.h>

/*
 * Structures to store domain infos. Must be here and not locally in function,
 * since the maximum stack size is 8 KB
 */
static struct domain_migration_info dom_info = {0};

void evtchn_bind_existing_interdomain(struct domain *ld, struct domain *remote, int lport, int rport);

/*------------------------------------------------------------------------------
build_domain_migration_info
build_vcpu_migration_info
    Build the structures holding the key info to be migrated over
------------------------------------------------------------------------------*/
static void build_domain_migration_info(unsigned int ME_slotID, struct domain *me, struct domain_migration_info *mig_info)
{

	/* Event channel info */
	memcpy(mig_info->evtchn, me->evtchn, sizeof(me->evtchn));

	/* Get the start_info structure */
	mig_info->avz_shared = *(me->avz_shared);

	/* Update the state for the ME instance which will migrate. The resident ME keeps its current state. */
	mig_info->avz_shared.dom_desc.u.ME.state = ME_state_migrating;

	mig_info->pause_count = me->pause_count;

	mig_info->need_periodic_timer = me->need_periodic_timer;

	/* Pause */
	mig_info->pause_flags = me->pause_flags;

	memcpy(&(mig_info->pause_count), &(me->pause_count), sizeof(me->pause_count));

	/* VIRQ mapping */
	memcpy(mig_info->virq_to_evtchn, me->virq_to_evtchn, sizeof((me->virq_to_evtchn)));

	/* Arch & address space */

	mig_info->cpu_regs = me->cpu_regs;

#ifdef CONFIG_ARCH_ARM32
	mig_info->vfp = me->vfp;
#endif
}

/**
 * Read the migration info structures.
 */
void read_migration_structures(avz_hyp_t *args) {
        unsigned int ME_slotID = args->u.avz_migstruct_read_args.slotID;
        struct domain *domME = domains[ME_slotID];

	/* Gather all the info we need into structures */
	build_domain_migration_info(ME_slotID, domME, &dom_info);

	/* Copy structures to buffer */
	memcpy((void *) ipa_to_va(MEMSLOT_AGENCY,
		args->u.avz_migstruct_read_args.migstruct_paddr), &dom_info, sizeof(dom_info));

	/* Update op->avz_shared with valid data size */
	args->u.avz_migstruct_read_args.size = sizeof(dom_info);
}

/*------------------------------------------------------------------------------
restore_domain_migration_info
restore_vcpu_migration_info
    Restore the migration info in the new ME structure
    Those function are actually exported and called in domain_migrate_restore.c
    They were kept in this file because they are the symmetric functions of
    build_domain_migration_info() and build_vcpu_migration_info()
------------------------------------------------------------------------------*/

static void restore_domain_migration_info(unsigned int ME_slotID, struct domain *me, struct domain_migration_info *mig_info)
{
	int i;
	
	DBG("%s\n", __func__);

	*(me->avz_shared) = mig_info->avz_shared;

	/* Check that our signature is valid so that the image transfer should be good. */
	if (strcmp(me->avz_shared->signature, SOO_ME_SIGNATURE))
		panic("%s: Cannot find the correct signature in the shared page (" SOO_ME_SIGNATURE ")...\n", __func__);

	/* Update the domID of course */
	me->avz_shared->domID = ME_slotID;

	memcpy(me->evtchn, mig_info->evtchn, sizeof(me->evtchn));

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

	me->avz_shared->printch = printch;
	me->pause_count = mig_info->pause_count;
	me->need_periodic_timer = mig_info->need_periodic_timer;

	/* Pause */
	me->pause_flags = mig_info->pause_flags;

	memcpy(&(me->pause_count), &(mig_info->pause_count), sizeof(me->pause_count));

	/* VIRQ mapping */
	memcpy(me->virq_to_evtchn, mig_info->virq_to_evtchn, sizeof((me->virq_to_evtchn)));

	/* Fields related to CPU */
	me->cpu_regs = mig_info->cpu_regs;
}


/**
 * Write the migration info structures.
 */
void write_migration_structures(avz_hyp_t *args) {

	/* Get the migration info structures */
	memcpy(&dom_info, (void *) ipa_to_va(MEMSLOT_AGENCY, 
		args->u.avz_migstruct_write_args.migstruct_paddr), sizeof(dom_info));
}

/**
 * Initiate the last stage of the migration process of a ME, so called "migration finalization".
 */
void migration_final(avz_hyp_t *args) {
        unsigned int slotID = args->u.avz_mig_final_args.slotID;
        struct domain *me = domains[slotID];

	DBG("ME state: %d\n", get_ME_state(slotID));

	switch (get_ME_state(slotID)) {

	case ME_state_suspended:
		DBG("ME state suspended\n");
		domain_unpause_by_systemcontroller(me);
		break;

	case ME_state_migrating:
		DBG("ME_state_migrating\n");

		flush_dcache_all();

		restore_migrated_domain(slotID);
		break;

	case ME_state_preparing:
		DBG("ME state preparing\n");

		flush_dcache_all();
		break;

	default:
		printk("avz: %s:%d Invalid state at this point (%d)\n", __func__, __LINE__, get_ME_state(slotID));
		BUG();

		break;
	}
}

/*------------------------------------------------------------------------------
 sync_directcomm
 This function updates the directcomm event channel in both domains
 ------------------------------------------------------------------------------*/
static void rebind_directcomm(unsigned int ME_slotID)
{
	struct domain *me = domains[ME_slotID];
	 
	DBG("Rebinding directcomm...\n");
#if 0
	/* Get the directcomm evtchn from agency */

	memset(&agency_directcomm_args, 0, sizeof(struct DOMCALL_directcomm_args));

	/* Pass the (remote) domID in directcomm_evtchn */
	agency_directcomm_args.directcomm_evtchn = ME_slotID;

	domain_call(agency, DOMCALL_sync_directcomm, &agency_directcomm_args);

	memset(&ME_directcomm_args, 0, sizeof(struct DOMCALL_directcomm_args));

	/* Pass the domID in directcomm_evtchn */
	ME_directcomm_args.directcomm_evtchn = 0;

	domain_call(me, DOMCALL_sync_directcomm, &ME_directcomm_args);

	DBG("[soo:avz] %s: Rebinding directcomm event channels: %d (agency) <-> %d (ME)\n", __func__, agency_directcomm_args.directcomm_evtchn, ME_directcomm_args.directcomm_evtchn);

	evtchn_bind_existing_interdomain(me, agency, ME_directcomm_args.directcomm_evtchn, agency_directcomm_args.directcomm_evtchn);
#endif
}

/*------------------------------------------------------------------------------
 sync_domain_interactions
 - Create the mmory mappings in ME which are normally done at boot time
   This is done using DOMCALLs. We first have to retrieve info from agency
   using DOMCALLs as well.
   We pass the current domain as argument as we need it to make the DOMCALLs.
 - Performs the rebinding of vbstore event channel
 ------------------------------------------------------------------------------*/
static void sync_domain_interactions(unsigned int ME_slotID)
{
	struct domain *me = domains[ME_slotID];
#if 0
	struct DOMCALL_sync_vbstore_args xs_args;
	struct DOMCALL_sync_domain_interactions_args sync_args;

	memset(&xs_args, 0, sizeof(struct DOMCALL_sync_vbstore_args));

	/* Retrieve ME vbstore info from the agency */

	/* Pass the ME_domID in vbstore_remote_ME_evtchn field */
	xs_args.vbstore_revtchn = me->avz_shared->domID;

	domain_call(agency, DOMCALL_sync_vbstore, &xs_args);

	/* Create the mappings in ME */
	sync_args.vbstore_pfn = xs_args.vbstore_pfn;

	domain_call(me, DOMCALL_sync_domain_interactions, &sync_args);

	/*
	 * Rebinding the event channel used to access vbstore in agency
	 */
	DBG("%s: Get the vbstore pfn %lx for this ME\n", __func__, xs_args.vbstore_pfn);
	DBG("%s: Rebinding vbstore event channels: %d (agency) <-> %d (ME)\n", __func__, xs_args.vbstore_revtchn, sync_args.vbstore_levtchn);

	evtchn_bind_existing_interdomain(me, agency, sync_args.vbstore_levtchn, xs_args.vbstore_revtchn);

	rebind_directcomm(ME_slotID);
#endif
}

void restore_migrated_domain(unsigned int ME_slotID) {
	struct domain *me = NULL;
	addr_t current_pgtable_paddr;

	DBG("Restoring migrated domain on ME_slotID: %d\n", ME_slotID);

	me = domains[ME_slotID];

	restore_domain_migration_info(ME_slotID, me, &dom_info);

	/* Init post-migration execution of ME */

	/* Stack pointer (r13) should remain unchanged since on the receiver side we did not make any push on the SVC stack */
	me->cpu_regs.sp = (unsigned long) setup_dom_stack(me);

#ifndef CONFIG_ARM64VT
	/* Setting the (future) value of PC in r14 (LR). See code switch_to in entry-armv.S */
	me->cpu_regs.lr = (unsigned long) (void *) after_migrate_to_user;
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

/**
 * Initiate the migration process of a ME.
 */
void migration_init(avz_hyp_t *args) {
        unsigned int slotID = args->u.avz_mig_init_args.slotID;
        struct domain *domME = domains[slotID];

        DBG("Initializing migration of ME slotID=%d\n", slotID);

	switch (get_ME_state(slotID)) {

	case ME_state_suspended:
		DBG("ME state suspended\n");

		/* Initiator's side: the ME must be suspended during the migration */
		domain_pause_by_systemcontroller(domME);

		DBG0("ME paused OK\n");
		
		break;

	case ME_state_booting:

		DBG("ME state booting\n");

		/* Initialize the ME descriptor */
		set_ME_state(slotID, ME_state_booting);

		/* Set the size of this ME in its own descriptor */
		domME->avz_shared->dom_desc.u.ME.size = memslot[slotID].size;

		/* Now set the pfn base of this ME; this will be useful for the Agency Core subsystem */
		domME->avz_shared->dom_desc.u.ME.pfn = phys_to_pfn(memslot[slotID].base_paddr);

		break;

	case ME_state_migrating:

		/* Target's side: nothing to do in particular */
		DBG("ME state migrating\n");

		/* Pre-init the basic information related to the ME */
		domME->avz_shared->dom_desc.u.ME.size = memslot[slotID].size;
		domME->avz_shared->dom_desc.u.ME.pfn = phys_to_pfn(memslot[slotID].base_paddr);

		break;

	default:
		printk("Agency: %s:%d Invalid state at this point (%d)\n", __func__, __LINE__, get_ME_state(slotID));
		BUG();
	}
}

