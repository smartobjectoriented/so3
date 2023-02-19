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
#include <avz/uapi/soo.h>
#endif

#include <asm/mmu.h>

#ifdef CONFIG_ARCH_ARM32
#include <asm/vfp.h>
#endif

/* We keep the STACK_SIZE to 8192 in order to have a similar stack_size as guest OS in SVC mode */
#define DOMAIN_STACK_SIZE  (PAGE_SIZE << 1)

#ifdef __ASSEMBLY__

.macro	curdom	rd, tmp

	// Compute the address of the stack bottom where cpu_info is located.
	ldr	\rd, =(~(DOMAIN_STACK_SIZE - 1))
	mov	\tmp, sp
	and	\rd, \tmp, \rd

	// Get the address of the domain descriptor
	ldr	\rd, [\rd]
.endm


#else /* __ASSEMBLY__ */

#include <spinlock.h>
#include <timer.h>

#include <avz/uapi/avz.h>

struct evtchn
{
	u8  state;             /* ECS_* */

	bool can_notify;

	struct {
		domid_t remote_domid;
	} unbound;     /* state == ECS_UNBOUND */

	struct {
		u16 remote_evtchn;
		struct domain *remote_dom;
	} interdomain; /* state == ECS_INTERDOMAIN */

	volatile bool pending;

	u16 virq;      /* state == ECS_VIRQ */

};

struct domain
{
	/* The spinlocks are placed here to have a 8-byte alignement
	 * required by ldaxr instruction.
	 */

	spinlock_t domain_lock;
	spinlock_t event_lock;
	spinlock_t virq_lock;

	/* Fields related to the underlying CPU */
	cpu_regs_t cpu_regs;
	addr_t   g_sp; 	/* G-stack */

	addr_t	event_callback;
	addr_t	domcall;

#ifdef CONFIG_ARCH_ARM32
	struct vfp_state vfp;
#endif
	avz_shared_t *avz_shared;     /* shared data area between AVZ and the domain */

	unsigned int max_pages;    /* maximum value for tot_pages        */

	/* Event channel information. */
	struct evtchn evtchn[NR_EVTCHN];
	u16 virq_to_evtchn[NR_VIRQS];

	/* Is this guest dying (i.e., a zombie)? */
	enum { DOMDYING_alive, DOMDYING_dying, DOMDYING_dead } is_dying;

	/* Domain is paused by controller software? */
	bool is_paused_by_controller;

	int processor;

	bool need_periodic_timer;
	struct timer oneshot_timer;

	struct scheduler *sched;

	int runstate;

	/* Currently running on a CPU? */
	bool is_running;

	unsigned long pause_flags;
	atomic_t pause_count;

	unsigned long domain_stack;
};


#define USE_NORMAL_PGTABLE	0
#define USE_SYSTEM_PGTABLE	1

extern struct domain *agency_rt_domain;
extern struct domain *domains[MAX_DOMAINS];

extern int construct_agency(struct domain *d);
extern int construct_ME(struct domain *d);

extern void new_thread(struct domain *d, unsigned long start_pc, unsigned long r2_arg, unsigned long start_stack);
void *setup_dom_stack(struct domain *d);

extern void domain_call(struct domain *target_dom, int cmd, void *arg);

void machine_halt(void);

void arch_domain_create(struct domain *d, int cpu_id);
void arch_setup_domain_frame(struct domain *d, cpu_regs_t *domain_frame, addr_t fdt_addr, addr_t start_stack, addr_t start_pc);

/*
 * setup_page_table_guestOS() is setting up the 1st-level and 2nd-level page tables within the domain.
 */
#ifdef CONFIG_ARM64VT
void __setup_dom_pgtable(struct domain *d, addr_t ipa_start, unsigned long map_size);
#else
void __setup_dom_pgtable(struct domain *d, addr_t v_start, unsigned long map_size, addr_t p_start);
#endif

/*
 * Arch-specifics.
 */

/* Allocate/free a domain structure. */
struct domain *alloc_domain_struct(void);
void free_domain_struct(struct domain *d);

/* Allocate/free a VCPU structure. */
struct vcpu *alloc_vcpu_struct(struct domain *d);

void free_vcpu_struct(struct vcpu *v);
void vcpu_destroy(struct vcpu *v);

void arch_domain_destroy(struct domain *d);

int domain_relinquish_resources(struct domain *d);

void dump_pageframe_info(struct domain *d);

void arch_dump_vcpu_info(struct vcpu *v);

void arch_dump_domain_info(struct domain *d);

void arch_vcpu_reset(struct vcpu *v);

#endif /* !__ASSEMBLY__ */

#endif /* DOMAIN_H */
