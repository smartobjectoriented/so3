/*
 * Copyright (C) 2016,2017 Daniel Rossier <daniel.rossier@soo.tech>
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
#include <soo/uapi/soo.h>
#endif

#include <asm/mmu.h>

/* We keep the STACK_SIZE to 8192 in order to have a similar stack_size as guest OS in SVC mode */
#define DOMAIN_STACK_SIZE (PAGE_SIZE << 1)

#ifdef __ASSEMBLY__

.macro curdom rd,
	tmp

		// Compute the address of the stack bottom where cpu_info is located.
		ldr	\rd,
	= (~(DOMAIN_STACK_SIZE - 1)) mov	\tmp,
	sp and	\rd, \tmp, \rd

		// Get the address of the domain descriptor
		ldr	\rd,
	[\rd].endm

#else /* __ASSEMBLY__ */

#include <spinlock.h>
#include <timer.h>
#include <list.h>

#include <avz/uapi/avz.h>

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

	avz_shared_t
		*avz_shared; /* shared data area between AVZ and the domain */

	/* Physical and virtual address of the page table used when the domain is bootstraping */
	addr_t pagetable_paddr;
	addr_t pagetable_vaddr; /* Required when bootstrapping the domain */

	unsigned int max_pages; /* maximum value for tot_pages */

	/* Event channel information. */
	struct evtchn evtchn[NR_EVTCHN];
	u16 virq_to_evtchn[NR_VIRQS];

	/* Is this guest dying (i.e., a zombie)? */
	enum { DOMDYING_alive, DOMDYING_dying, DOMDYING_dead } is_dying;

	/* Domain is paused by controller software? */
	bool is_paused_by_controller;

	/* Grant table to store the pages granted by this domain to the other */
	struct list_head gnttab;

	/* IPA reserved page frame numbers for mapping granted pages belonging to other domains */
	grant_pfn_t grant_pfn[NR_GRANT_PFN];

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

extern struct domain *agency_rt_domain;
extern struct domain *domains[MAX_DOMAINS];

extern int construct_agency(struct domain *d);
extern int construct_ME(struct domain *d);

ME_state_t get_ME_state(unsigned int ME_slotID);

void do_domctl(domctl_t *args);

void *setup_dom_stack(struct domain *d);

void machine_halt(void);

void arch_domain_create(struct domain *d, int cpu_id);

void initialize_hyp_dom_stack(struct domain *d, addr_t fdt_paddr,
			      addr_t entry_addr);

/*
 * setup_page_table_guestOS() is setting up the 1st-level and 2nd-level page tables within the domain.
 */

void __setup_dom_pgtable(struct domain *d, addr_t ipa_start,
			 unsigned long map_size);

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
