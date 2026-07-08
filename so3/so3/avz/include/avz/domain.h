/*
 * Copyright (C) 2016-2026 Daniel Rossier <daniel.rossier@soo.tech>
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

#ifndef DOMAIN_H
#define DOMAIN_H

#ifndef __ASSEMBLY__
#ifdef CONFIG_SOO
#include <soo/uapi/soo.h>
#else
#include <avz/uapi/avz.h>
#endif
#endif

#include <asm/mmu.h>

/* We keep the STACK_SIZE to 8192 in order to have a similar stack_size as guest OS in SVC mode */
#define DOMAIN_STACK_SIZE (PAGE_SIZE << 1)

#ifdef __ASSEMBLY__

/* clang-format off */
.macro curdom rd, tmp

	// Compute the address of the stack bottom where cpu_info is located.
	ldr	\rd, = (~(DOMAIN_STACK_SIZE - 1)) 
	mov	\tmp, sp 
	and	\rd, \tmp, \rd

	// Get the address of the domain descriptor
	ldr	\rd, [\rd]
.endm
/* clang-format on */

#else /* __ASSEMBLY__ */

#include <spinlock.h>
#include <timer.h>
#include <list.h>

#define NR_GRANT_PFN 32

typedef struct {
	addr_t pfn;
	bool free;
} grant_pfn_t;

struct evtchn {
	u8 state; /* ECS_* */

	bool can_notify;

	struct {
		domid_t remote_domid;
	} unbound; /* state == ECS_UNBOUND */

	struct {
		u16 remote_evtchn;
		struct domain *remote_dom;
	} interdomain; /* state == ECS_INTERDOMAIN */

	volatile bool pending;

	u16 virq; /* state == ECS_VIRQ */
};

struct domain {
	/* The spinlocks are placed here to have a 8-byte alignement
	 * required by ldaxr instruction.
	 */

	spinlock_t domain_lock;
	spinlock_t event_lock;
	spinlock_t virq_lock;

	vcpu_t vcpu;

	addr_t event_callback;
	addr_t domcall;

	avz_shared_t *avz_shared; /* shared data area between AVZ and the domain */

	/* Physical and virtual address of the page table used when the domain is bootstraping.
	 * VTCR_EL2 runs with SL0=L1, so pagetable_paddr/vaddr refer to the L1 table
	 * (the value loaded in VTTBR_EL2), NOT the L0 root. */
	addr_t pagetable_paddr;
	addr_t pagetable_vaddr; /* Required when bootstrapping the domain */

	/* L0 root of the stage-2 tables. Walk helpers (__create_mapping) expect
	 * the L0 root — use this one, not pagetable_vaddr, to add S2 mappings
	 * (grant pages, vbstore ring, fbdev) after the domain is set up. */
	addr_t pagetable_l0_vaddr;

	unsigned int max_pages; /* maximum value for tot_pages */

	/* Event channel information. */
	struct evtchn evtchn[NR_EVTCHN];
	u16 virq_to_evtchn[NR_VIRQS];

	/* Is this guest dying (i.e., a zombie)? */
	enum { DOMDYING_alive, DOMDYING_dying, DOMDYING_dead } is_dying;

	/* Domain is paused by controller software? */
	bool is_paused_by_controller;

#ifdef CONFIG_SOO
	/* Grant table to store the pages granted by this domain to the other */
	struct list_head gnttab;

	/* IPA reserved page frame numbers for mapping granted pages belonging to other domains */
	grant_pfn_t grant_pfn[NR_GRANT_PFN];

	/* IPA reserved starting page frame number for framebuffer mapping */
	addr_t fbdev_start_pfn;
#endif /* CONFIG_SOO */

	int processor;

	bool need_periodic_timer;
	struct timer oneshot_timer;

	struct scheduler *sched;

	int runstate;

	/* Currently running on a CPU? */
	bool is_running;

	unsigned long pause_flags;
	atomic_t pause_count;

	/* Hypervisor stack for this domain */
	void *domain_stack;
};

#define USE_NORMAL_PGTABLE 0
#define USE_SYSTEM_PGTABLE 1

#ifdef CONFIG_SOO
extern struct domain *domains[MAX_DOMAINS];
#endif /* CONFIG_SOO */

extern int construct_agency(struct domain *d);

#ifdef CONFIG_SOO
extern int construct_S3C(struct domain *d);
S3C_state_t get_S3C_state(unsigned int S3C_slotID);
#endif /* CONFIG_SOO */

void do_domctl(domctl_t *args);

void *setup_dom_stack(struct domain *d);

void machine_halt(void);

void arch_domain_create(struct domain *d, int cpu_id);

void initialize_hyp_dom_stack(struct domain *d, addr_t fdt_paddr, addr_t entry_addr);

/*
 * setup_page_table_guestOS() is setting up the 1st-level and 2nd-level page tables within the domain.
 */

void __setup_dom_pgtable(struct domain *d, addr_t ipa_start, unsigned long map_size);

void domain_unpause_by_systemcontroller(struct domain *d);

/* Allocate/free a domain structure. */
struct domain *alloc_domain_struct(void);
void free_domain_struct(struct domain *d);

/* Allocate/free a VCPU structure. */
struct vcpu *alloc_vcpu_struct(struct domain *d);

void free_vcpu_struct(struct vcpu *v);
void vcpu_destroy(struct vcpu *v);

void arch_domain_destroy(struct domain *d);

void arch_dump_vcpu_info(struct vcpu *v);

void arch_dump_domain_info(struct domain *d);

void arch_vcpu_reset(struct vcpu *v);

#endif /* !__ASSEMBLY__ */

#endif /* DOMAIN_H */
