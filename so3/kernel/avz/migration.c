
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
#if 0
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

#include <avz/uapi/event_channel.h>
#include <avz/uapi/soo.h>

#include <libfdt/image.h>

#include <libfdt/libfdt.h>

#include <avz/uapi/soo.h>
#include <avz/uapi/avz.h>

#include <avz/debug.h>
#include <avz/logbool.h>

/* PFN offset on the target platform */
volatile long pfn_offset = 0;

/* Start of ME RAM in virtual address space of idle domain (extern) */
unsigned long vaddr_start_ME = 0;

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
	mig_info->g_sp = me->g_sp;

#ifdef CONFIG_ARCH_ARM32
	mig_info->vfp = me->vfp;
#endif
}

/**
 * Read the migration info structures.
 */
void read_migration_structures(soo_hyp_t *op) {
	unsigned int ME_slotID = *((unsigned int *) op->p_val1);
	struct domain *domME = domains[ME_slotID];

	/* Gather all the info we need into structures */
	build_domain_migration_info(ME_slotID, domME, &dom_info);

	/* Copy structures to buffer */
	memcpy((void *) op->vaddr, &dom_info, sizeof(dom_info));

	/* Update op->avz_sharedze with valid data size */
	*((unsigned int *) op->p_val2) = sizeof(dom_info);
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
	void *logbool_ht;

	DBG("%s\n", __func__);

	/* For the time being, the logbool hashtable is re-initialized in the migrating ME. */
	logbool_ht = me->avz_shared->logbool_ht;

	*(me->avz_shared) = mig_info->avz_shared;

	me->avz_shared->logbool_ht = logbool_ht;

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

	pfn_offset = phys_to_pfn(me->avz_shared->dom_phys_offset) - phys_to_pfn(mig_info->avz_shared.dom_phys_offset);

	me->avz_shared->hypercall_vaddr = (unsigned long) hypercall_entry;

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
	me->g_sp = mig_info->g_sp;

#ifdef CONFIG_ARCH_ARM32
	me->vfp = mig_info->vfp;
#endif
}


/**
 * Write the migration info structures.
 */
void write_migration_structures(soo_hyp_t *op) {

	/* Get the migration info structures */
	memcpy(&dom_info, (void *) op->vaddr, sizeof(dom_info));
}

/**
 * Initiate the last stage of the migration process of a ME, so called "migration finalization".
 */
void migration_final(soo_hyp_t *op) {
	unsigned int slotID = *((unsigned int *) op->p_val1);
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

		DBG("Calling cooperate()...\n");

		soo_cooperate(me->avz_shared->domID);
		break;

	default:
		printk("Agency: %s:%d Invalid state at this point (%d)\n", __func__, __LINE__, get_ME_state(slotID));
		BUG();

		break;
	}
}

/*------------------------------------------------------------------------------
 fix_page_tables_ME
 Fix all page tables in ME (swapper_pg_dir + all processes)
 We pass the current domain as argument as we need it to make the DOMCALLs.
 ------------------------------------------------------------------------------*/
static void fix_other_page_tables_ME(unsigned int ME_slotID)
{
	struct domain *me = domains[ME_slotID];
	struct DOMCALL_fix_page_tables_args fix_pt_args;

	fix_pt_args.pfn_offset = pfn_offset;

	fix_pt_args.min_pfn =  me->avz_shared->dom_phys_offset >> PAGE_SHIFT;
	fix_pt_args.nr_pages = me->avz_shared->nr_pages;

	DBG("DOMCALL_fix_other_page_tables called in ME with pfn_offset=%ld (%lx)\n", fix_pt_args.pfn_offset, fix_pt_args.pfn_offset);

	domain_call(me, DOMCALL_fix_other_page_tables, &fix_pt_args);

	/* Flush all cache */
	flush_dcache_all();
}

/*------------------------------------------------------------------------------
 sync_directcomm
 This function updates the directcomm event channel in both domains
 ------------------------------------------------------------------------------*/
static void rebind_directcomm(unsigned int ME_slotID)
{
	struct domain *me = domains[ME_slotID];
	struct DOMCALL_directcomm_args agency_directcomm_args, ME_directcomm_args;

	DBG("Rebinding directcomm...\n");

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
}

/*------------------------------------------------------------------------------
 adjust_variables_in_ME
 Adjust variables such as start_info in ME
 ------------------------------------------------------------------------------*/
static void presetup_adjust_variables_in_ME(unsigned int ME_slotID, avz_shared_t *avz_shared)
{
	struct domain *me = domains[ME_slotID];
	struct DOMCALL_presetup_adjust_variables_args adjust_variables;

	adjust_variables.avz_shared = avz_shared;

	domain_call(me, DOMCALL_presetup_adjust_variables, &adjust_variables);
}

/*------------------------------------------------------------------------------
 adjust_variables_in_ME
 Adjust variables such as start_info in ME
 ------------------------------------------------------------------------------*/
static void postsetup_adjust_variables_in_ME(unsigned int ME_slotID)
{
	struct domain *me = domains[ME_slotID];
	struct DOMCALL_postsetup_adjust_variables_args adjust_variables;

	adjust_variables.pfn_offset = pfn_offset;

	domain_call(me, DOMCALL_postsetup_adjust_variables, &adjust_variables);
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
	mmu_switch_kernel((void *) idle_domain[smp_processor_id()]->avz_shared->pagetable_paddr);

	/* Fix the ME kernel page table for domcalls to work */
	fix_kernel_boot_page_table_ME(ME_slotID);

	DBG0("DOMCALL_presetup_adjust_variables_in_ME\n");

	/* Adjust variables in ME such as start_info */
	presetup_adjust_variables_in_ME(ME_slotID, me->avz_shared);

	/* Fix all page tables in the ME (all processes) via a domcall */
	DBG("%s: fix other page tables in the ME...\n", __func__);
	fix_other_page_tables_ME(ME_slotID);

	DBG0("DOMCALL_postsetup_adjust_variables_in_ME\n");

	/* Adjust variables in the ME such as re-adjusting pfns */
	postsetup_adjust_variables_in_ME(ME_slotID);

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

	/* Proceed with the SOO post-migration callbacks according to patent */

	/* Pre-activate */
	soo_pre_activate(ME_slotID);

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

	/*
	 * Cooperate.
	 * We look for residing MEs which are ready to collaborate.
	 */

	soo_cooperate(ME_slotID);

	/*
	 * We check if the ME has been killed or set to the dormant state during the cooperate
	 * callback. If yes, we do not pursue our re-activation process.
	 */
	if ((domains[ME_slotID] == NULL) || (get_ME_state(ME_slotID) == ME_state_dead) || (get_ME_state(ME_slotID) == ME_state_dormant)) {

		set_current_domain(agency);
		mmu_switch_kernel((void *) current_pgtable_paddr);

		return ;
	}

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
void migration_init(soo_hyp_t *op) {
	unsigned int slotID = *((unsigned int *) op->p_val1);
	struct domain *domME = domains[slotID];

	DBG("Initializing migration of ME slotID=%d\n", slotID);

	switch (get_ME_state(slotID)) {

	case ME_state_suspended:
		DBG("ME state suspended\n");

		/* Initiator's side: the ME must be suspended during the migration */
		domain_pause_by_systemcontroller(domME);

		DBG0("ME paused OK\n");
		DBG("Being migrated: preparing to copy in ME_slotID %d: ME @ paddr 0x%08x (mapped @ vaddr 0x%08x in eventhypervisor)\n",
			slotID, (unsigned int) memslot[slotID].base_paddr, (unsigned int) vaddr_start_ME);

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

	/* Used for future restore operation */
	vaddr_start_ME  = (unsigned long) __lva(memslot[slotID].base_paddr);
}

