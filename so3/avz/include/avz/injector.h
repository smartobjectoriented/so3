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

	/* IPA reserved start frame number for framebuffer */
	addr_t fbdev_start_pfn;

	/* Stack frame of this domain */
	struct cpu_regs stack_frame;

	/* Fields related to the CPU state */
	vcpu_t vcpu;
};

/* Load a capsule into a moemory slot */
void inject_capsule(avz_hyp_t *args);

/* Start the execution of a capsule */
void start_capsule(avz_hyp_t *args);

void read_ME_snapshot(avz_hyp_t *args);
void write_ME_snapshot(avz_hyp_t *args);

#endif /* INJECTOR_H */