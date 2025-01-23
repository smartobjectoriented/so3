
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

		//restore_migrated_domain(slotID);
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

void sync_domain_interactions(unsigned int ME_slotID)
{
	struct domain *me = domains[ME_slotID];
#if 0
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

		domain_pause_by_systemcontroller(domME);

		DBG0("ME paused OK\n");
		
		break;

	case ME_state_booting:

		DBG("ME state booting\n");

		/* Initialize the ME descriptor */
		set_ME_state(slotID, ME_state_booting);

		/* Set the size of this ME in its own descriptor */
		domME->avz_shared->dom_desc.u.ME.size = memslot[slotID].size;

		break;

	case ME_state_migrating:

		/* Target's side: nothing to do in particular */
		DBG("ME state migrating\n");

		/* Pre-init the basic information related to the ME */
		domME->avz_shared->dom_desc.u.ME.size = memslot[slotID].size;
	
		break;

	default:
		printk("Agency: %s:%d Invalid state at this point (%d)\n", __func__, __LINE__, get_ME_state(slotID));
		BUG();
	}
}

