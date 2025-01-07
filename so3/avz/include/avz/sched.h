/*
 * Copyright (C) 2016 Daniel Rossier <daniel.rossier@soo.tech>
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

#ifndef __SCHED_H__
#define __SCHED_H__

#include <common.h>
#include <types.h>
#include <spinlock.h>
#include <smp.h>
#include <timer.h>
#include <memory.h>

#include <device/irq.h>

#include <avz/vcpu.h>
#include <avz/domain.h>

#include <avz/uapi/avz.h>

/*
 * Voluntarily yield the CPU.
 * @arg == NULL.
 */
#define SCHEDOP_yield       0

/*
 * Block execution of this VCPU until an event is received for processing.
 * If called with event upcalls masked, this operation will atomically
 * reenable event delivery and check for pending events before blocking the
 * VCPU. This avoids a "wakeup waiting" race.
 * @arg == NULL.
 */
#define SCHEDOP_block       1

/* A global pointer to the initial domain (Agency). */
extern struct domain *agency;
extern struct domain *domains[];

DECLARE_PER_CPU(struct domain *, current_domain);

void  evtchn_init(struct domain *d); /* from domain_create */
void evtchn_destroy(struct domain *d); /* from domain_kill */
void evtchn_destroy_final(struct domain *d); /* from complete_domain_destroy */

/* Per-domain lock can be recursively acquired in fault handlers. */
#define domain_lock(d) spin_lock_recursive(&(d)->domain_lock)
#define domain_unlock(d) spin_unlock_recursive(&(d)->domain_lock)
#define domain_is_locked(d) spin_is_locked(&(d)->domain_lock)

#define is_idle_domain(d) ((d)->avz_shared->domID == DOMID_IDLE)

#define DOMAIN_DESTROYED (1<<31) /* assumes atomic_t is >= 32 bits */

/*
 * Creation of new domain context associated to the agency or a Mobile Entity.
 * @domid is the domain number
 * @partial tells if the domain creation remains partial, without the creation of the vcpu structure which may intervene in a second step
 */
struct domain *domain_create(domid_t domid, int cpu_id);

void domain_destroy(struct domain *d);
void domain_shutdown(struct domain *d, u8 reason);
void domain_resume(struct domain *d);

extern void periodic_timer_work(struct domain *);

#define set_current_state(_s) do { current->state = (_s); } while (0)
void domain_scheduler_init(void);

int  sched_init_domain(struct domain *d, unsigned int processor);
void sched_destroy_domain(struct domain *d);

void vcpu_wake(struct domain *d);
void vcpu_sleep_nosync(struct domain *d);
void vcpu_sleep_sync(struct domain *d);


/*
 * Called by the scheduler to switch to another VCPU. This function must
 * call context_saved(@prev) when the local CPU is no longer running in
 * @prev's context, and that context is saved to memory. Alternatively, if
 * implementing lazy context switching, it suffices to ensure that invoking
 * sync_vcpu_execstate() will switch and commit @prev's state.
 */
void context_switch(struct domain *prev, struct domain *next);

void startup_cpu_idle_loop(void);

static inline void set_current_domain(struct domain *d) {
	per_cpu(current_domain, smp_processor_id()) = d;
}

static inline struct domain *current_domain(void) {
	return per_cpu(current_domain, smp_processor_id());
}

#define current_domain current_domain()

/*
 * Per-VCPU pause flags.
 */
 /* Domain is blocked waiting for an event. */
#define _VPF_blocked         0
#define VPF_blocked          (1UL<<_VPF_blocked)

/* VCPU is offline. */
#define _VPF_down            1
#define VPF_down             (1UL<<_VPF_down)

void vcpu_unblock(struct domain *v);
void vcpu_pause(struct domain *v);
void vcpu_pause_nosync(struct domain *v);
void domain_pause(struct domain *d);
void vcpu_unpause(struct domain *d);
void domain_unpause(struct domain *d);
void domain_pause_by_systemcontroller(struct domain *d);
void domain_unpause_by_systemcontroller(struct domain *d);

struct task_slice flip_do_schedule(void);

#endif /* __SCHED_H__ */

