/*
 * Copyright (C) 2024-2025 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef INJECTOR_H
#define INJECTOR_H

#include <avz/sched.h>

#include <avz/uapi/avz.h>

struct dom_context {
	/*
	 *  Event channel struct information.
	 */
	struct evtchn evtchn[NR_EVTCHN];

	/*
	 * Interrupt to event-channel mappings. Updates should be protected by the
	 * domain's event-channel spinlock. Read accesses can also synchronise on
	 * the lock, but races don't usually matter.
	 */
	unsigned int nr_pirqs;

	bool evtchn_pending[NR_EVTCHN];

	/* Start info page */
	avz_shared_t avz_shared;

	bool need_periodic_timer;

	unsigned long pause_flags;
	atomic_t pause_count;

	/* IRQ-safe virq_lock protects against delivering VIRQ to stale evtchn. */
	u16 virq_to_evtchn[NR_VIRQS];

	/* IPA physical address */
	addr_t ipa_addr;

	/* IPA reserved page frame numbers for granted pages */
	grant_pfn_t grant_pfn[NR_GRANT_PFN];

	/* Stack frame of this domain */
	struct cpu_regs stack_frame;

	/* Fields related to the CPU state */
	vcpu_t vcpu;
};

void mig_restore_domain_migration_info(unsigned int ME_slotID,
				       struct domain *me);
void after_migrate_to_user(void);

void migration_init(avz_hyp_t *args);
void migration_final(avz_hyp_t *args);

void read_migration_structures(avz_hyp_t *args);
void write_migration_structures(avz_hyp_t *args);

void restore_migrated_domain(unsigned int ME_slotID);
void restore_injected_domain(unsigned int ME_slotID);

void inject_me(avz_hyp_t *args);

#endif /* INJECTOR_H */