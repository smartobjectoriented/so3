/*
 * Copyright (C) 2014-2018 Daniel Rossier <daniel.rossier@soo.tech>
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

#ifndef __MIGRATION_H__
#define __MIGRATION_H__

#include <avz/sched.h>

#include <avz/uapi/avz.h>

struct domain_migration_info
{
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

	/* Fields related to the CPU state */
	cpu_regs_t cpu_regs;
	addr_t   g_sp; 	/* G-stack */

#ifdef CONFIG_ARCH_ARM32
	struct vfp_state vfp;
#endif
};

void mig_restore_domain_migration_info(unsigned int ME_slotID, struct domain *me);
void after_migrate_to_user(void);

void migration_init(soo_hyp_t *op);
void migration_final(soo_hyp_t *op);

void read_migration_structures(soo_hyp_t *op);
void write_migration_structures(soo_hyp_t *op);

void restore_migrated_domain(unsigned int ME_slotID);
void restore_injected_domain(unsigned int ME_slotID);

void inject_me(soo_hyp_t *op);

#endif /* __MIGRATION_H__ */
