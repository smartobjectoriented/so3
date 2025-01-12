
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
#include <memory.h>

#include <asm/cacheflush.h>
#include <asm/migration.h>

#include <device/irq.h>

#include <avz/memslot.h>
#include <avz/domain.h>
#include <avz/debug.h>
#include <avz/soo.h>

#include <libfdt/image.h>
#include <libfdt/libfdt.h>

#include <avz/uapi/avz.h>

void evtchn_bind_existing_interdomain(struct domain *ld, struct domain *remote, int lport, int rport);

void restore_migrated_domain(unsigned int ME_slotID);

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
void sync_domain_interactions(unsigned int ME_slotID)
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

void domain_pause_by_systemcontroller(struct domain *d);

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

