/*
 * Copyright (C) 2014-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <common.h>
#include <timer.h>
#include <softirq.h>
#include <spinlock.h>

#include <device/irq.h>

#include <device/arch/gic.h>

#include <asm/arm_timer.h>

#include <avz/evtchn.h>
#include <avz/sched.h>
#include <avz/sched-if.h>
#include <avz/domain.h>

DEFINE_PER_CPU(struct domain *, current_domain);

static spinlock_t schedule_lock;

/*
 * The scheduler lock this CPU holds, if any. A CPU enters a scheduler's paths
 * holding its lock from an outer one, and must then neither take it again nor
 * release it under that outer path. Whether the lock is held says nothing
 * about who holds it: an agency CPU pausing a capsule for a snapshot meets the
 * capsule CPU inside its scheduler, and each must wait for the other.
 */

static DEFINE_PER_CPU(spinlock_t *, sched_lock_held);
static DEFINE_PER_CPU(unsigned long, sched_lock_waited);

/**
 * Take a scheduler's lock, unless this CPU holds it already.
 *
 * @return true when it was taken here, for sched_unlock()
 */
bool sched_lock(struct schedule_data *sd)
{
	unsigned long flags;

	if (this_cpu(sched_lock_held) == &sd->schedule_lock)
		return false;

	if (spin_is_locked(&sd->schedule_lock))
		this_cpu(sched_lock_waited)++;

	/*
	 * The lock and the mark of its holder change together as seen from
	 * this CPU's interrupts: the scheduling softirq reads the mark to know
	 * whether the lock is its own.
	 */

	flags = spin_lock_irqsave(&sd->schedule_lock);
	this_cpu(sched_lock_held) = &sd->schedule_lock;
	local_irq_restore(flags);

	return true;
}

/**
 * Release a scheduler's lock taken by sched_lock() on this CPU; nothing when
 * it was held from an outer path, or released by a context switch since.
 */
void sched_unlock(struct schedule_data *sd, bool taken)
{
	unsigned long flags;

	if (!taken || (this_cpu(sched_lock_held) != &sd->schedule_lock))
		return;

	flags = local_irq_save();
	this_cpu(sched_lock_held) = NULL;
	spin_unlock(&sd->schedule_lock);
	local_irq_restore(flags);
}

/**
 * How many times a CPU found a scheduler's lock held by another CPU and
 * waited for it, over all CPUs.
 */
unsigned long sched_lock_contended(void)
{
	unsigned long n = 0;
	int i;

	for (i = 0; i < CONFIG_NR_CPUS; i++)
		n += per_cpu(sched_lock_waited, i);

	return n;
}

/* Various timer handlers. */

inline void domain_runstate_change(struct domain *d, int new_state)
{
	/*
	 * We might already be in RUNSTATE_blocked before setting to this state; for example,
	 * if a capsule has been paused and is killed during the cooperation phase,
	 * the call to shutdown() will lead to be here with such a state already.
	 */
	ASSERT((d->runstate == RUNSTATE_blocked) || (d->runstate != new_state));

	d->runstate = new_state;
}

int sched_init_domain(struct domain *d, unsigned int processor)
{
	unsigned int rc = 0;

	/* Idle VCPUs are scheduled immediately. */
	if (is_idle_domain(d))
		d->is_running = 1;

	return rc;
}

void vcpu_wake(struct domain *d)
{
	bool taken = sched_lock(&d->sched->sched_data);

	if (d->runstate >= RUNSTATE_blocked) {
		domain_runstate_change(d, RUNSTATE_runnable);

		d->sched->wake(d);

		clear_bit(_VPF_blocked, &d->pause_flags);
	}

	sched_unlock(&d->sched->sched_data, taken);
}

/**
 * Take a capsule idle in WFI off its scheduler, when another one would run:
 * its CPU goes to that one instead of waiting in the capsule's WFI until its
 * time slice ends. Its next event puts it back (vcpu_unpark). It stays
 * runnable for its timer, whose tick is such an event.
 *
 * Alone on its CPU, it stays: switching to the idle domain and back at each
 * event costs more than waiting in its place.
 *
 * @param d	the capsule, the current domain
 * @return true if parked, false if the caller waits for its event in WFI
 */
bool vcpu_park(struct domain *d)
{
	if (!d->sched->runnable || d->sched->runnable() <= 1)
		return false;

	d->sched->sleep(d);

	smp_mb();
	atomic_set(&d->parked, 1);
	smp_mb();

	/* An event which came meanwhile found it not parked yet. */

	if (d->avz_shared->evtchn_upcall_pending)
		vcpu_unpark(d);

	return true;
}

/**
 * Put a parked capsule back on its scheduler, which its CPU runs at once.
 * The flag decides who wakes it; a capsule paused meanwhile, its memory
 * being read for a snapshot, stays off: vcpu_unpause() wakes it.
 *
 * @param d	the capsule
 */
void vcpu_unpark(struct domain *d)
{
	bool taken;

	if (atomic_cmpxchg(&d->parked, 1, 0) != 1)
		return;

	taken = sched_lock(&d->sched->sched_data);

	if (!atomic_read(&d->pause_count))
		d->sched->wake(d);

	sched_unlock(&d->sched->sched_data, taken);
}

/**
 * @brief Save the CPU-related parameters which are specific to the domain
 * 	  The TTBRx registers are updated during memory context switch.
 * @param d 
 */
void vcpu_save_context(struct domain *d)
{
	d->vcpu.sctlr_el1 = read_sysreg(sctlr_el1);
	d->vcpu.vbar_el1 = read_sysreg(vbar_el1);
	d->vcpu.ttbr0_el1 = read_sysreg(ttbr0_el1);
	d->vcpu.ttbr1_el1 = read_sysreg(ttbr1_el1);
	d->vcpu.tcr_el1 = read_sysreg(tcr_el1);
	d->vcpu.mair_el1 = read_sysreg(mair_el1);
}

/**
 * @brief Restore the CPU-related parameters which are specific to the domain
 * 
 * @param d 
 */
void vcpu_restore_context(struct domain *d)
{
	/* Restore the CPU control register state */
	write_sysreg(d->vcpu.sctlr_el1, sctlr_el1);
	write_sysreg(d->vcpu.vbar_el1, vbar_el1);
	write_sysreg(d->vcpu.ttbr0_el1, ttbr0_el1);
	write_sysreg(d->vcpu.ttbr1_el1, ttbr1_el1);
	write_sysreg(d->vcpu.tcr_el1, tcr_el1);
	write_sysreg(d->vcpu.mair_el1, mair_el1);

	gic_clear_pending_irqs();

	/* Check for pending vIRQs for this domain */
	if (current_domain->avz_shared->evtchn_upcall_pending)
		gic_set_pending(IPI_EVENT_CHECK);
}

void sched_destroy_domain(struct domain *d)
{
	kill_timer(&d->oneshot_timer);
}

void vcpu_sleep_nosync(struct domain *d)
{
	bool taken = sched_lock(&d->sched->sched_data);

	/*
	 * Stop associated timers.
	 * This code is executed by CPU #0. It means that we have to take care about potential
	 * re-insertion of new timers by CPU #3 activities after the oneshot_timer has been stopped.
	 * For this reason, we first set the VPF_blocked bit to 1, and the sched_deadline/sched_sleep have
	 * to check for this bit.
	 */
	set_bit(_VPF_blocked, &d->pause_flags);

	/* Parked, its next event must not put it back: vcpu_wake() will. */

	atomic_set(&d->parked, 0);

	/* Now, setting the domain to blocked state */
	if (d->runstate != RUNSTATE_running)
		domain_runstate_change(d, RUNSTATE_blocked);

	if (active_timer(&d->oneshot_timer))
		stop_timer(&d->oneshot_timer);

	d->sched->sleep(d);

	sched_unlock(&d->sched->sched_data, taken);
}

/*
 * Set a domain sleeping. The domain state is set to blocked, and the caller
 * waits for the domain to leave its CPU, which the sleep asked that CPU to
 * schedule for. A wait beyond any scheduling tick is a stall: it is told
 * once, with what each CPU is at, and waited on all the same.
 */
void vcpu_sleep_sync(struct domain *d)
{
	u64 since = arch_counter_get_cntvct();
	bool told = false;

	vcpu_sleep_nosync(d);

	while (d->is_running) {
		cpu_relax();

		if (!told && (arch_counter_get_cntvct() - since > 2 * (u64) arch_timer_get_cntfrq())) {
			told = true;
			printk("%s: CPU%d has waited 2 s for domain %d to leave CPU%d: runstate %d, pause count %d, CPU%d runs domain %d, scheduler lock %s, lock waits %lu\n",
			       __func__, smp_processor_id(), d->avz_shared->domID, d->processor, d->runstate,
			       atomic_read(&d->pause_count), d->processor,
			       per_cpu(current_domain, d->processor) ?
				       per_cpu(current_domain, d->processor)->avz_shared->domID :
				       -1,
			       spin_is_locked(&d->sched->sched_data.schedule_lock) ? "held" : "free", sched_lock_contended());
		}
	}
}

/*
 * Voluntarily yield the processor to anther domain on this CPU.
 */
static long do_yield(void)
{
	raise_softirq(SCHEDULE_SOFTIRQ);

	return 0;
}

long do_sched_op(int cmd, void *args)
{
	long ret = 0;

	switch (cmd) {
	case SCHEDOP_yield:
		ret = do_yield();
		break;
	}

	return ret;
}

/* 
 * The main function
 * - deschedule the current domain (scheduler independent).
 * - pick a new domain (scheduler dependent).
 */
static void domain_schedule(void)
{
	struct domain *prev = current_domain, *next = NULL;
	struct schedule_data *sd;
	struct task_slice next_slice;

	ASSERT(local_irq_is_disabled());
	ASSERT(prev->runstate == RUNSTATE_running);

	if (!prev->sched) {
		while (1)
			cpu_do_idle();
	}

	/* Against the other CPUs, and not again when an outer path on this CPU
	 * holds it: the context switch, or the no-op path below, releases it.
	 */

	sched_lock(&current_domain->sched->sched_data);

	sd = &current_domain->sched->sched_data;

	if (active_timer(&sd->s_timer))
		stop_timer(&sd->s_timer);

	/* get policy-specific decision on scheduling... */
	next_slice = prev->sched->do_schedule();

	next = next_slice.d;

#ifdef CONFIG_SOO
	if (next_slice.time > 0ull)
		set_timer(&next->sched->sched_data.s_timer, NOW() + MILLISECS(next_slice.time));
#endif /* CONFIG_SOO */

	if (unlikely(prev == next)) {
		sched_unlock(&prev->sched->sched_data, true);
		ASSERT(prev->runstate == RUNSTATE_running);

		return;
	}
	ASSERT(prev->runstate == RUNSTATE_running);

	domain_runstate_change(prev, (test_bit(_VPF_blocked, &prev->pause_flags) ?
					      RUNSTATE_blocked :
					      (prev->is_dying ? RUNSTATE_offline : RUNSTATE_runnable)));

	ASSERT(next->runstate != RUNSTATE_running);
	domain_runstate_change(next, RUNSTATE_running);

	ASSERT(!next->is_running);
	next->is_running = 1;

#if 0 /* debug */
	printk("### running on cpu: %d prev: %d next: %d\n", smp_processor_id(), prev->avz_shared->domID, next->avz_shared->domID);
#endif /* 0 */

	/* We do not unlock the schedulder_lock until everything has been processed */

	context_switch(prev, next);

	/* From here, prev and next are those in the current domain; don't forget ;-) */
}

/** Just to bootstrap the agency **/
static struct task_slice agency_schedule(void)
{
	struct task_slice ts;

	/* No domain must be scheduled on the agency CPUs: they run the agency
	 * guest under stage-2. The secondary agency CPUs ERET into the guest
	 * before init_idle_domain() runs, so they have NO idle domain at all:
	 * a (spurious) SCHEDULE_SOFTIRQ processed at EL2 on such a CPU (e.g.
	 * raised by a capsule-management hypercall issued from that CPU) used
	 * to pick the NULL idle domain and die on a data abort at EL2, killing
	 * the CPU (stuck in the trap handler) and wedging the agency. Return
	 * the agency domain instead: domain_schedule() then takes its
	 * prev == next no-op path and the CPU goes back to the guest.
	 */

	if ((smp_processor_id() == AGENCY_CPU) || (idle_domain[smp_processor_id()] == NULL))
		ts.d = domains[0];
	else
		ts.d = idle_domain[smp_processor_id()];

	ts.time = 0;

	return ts;
}

static void agency_wake(struct domain *d)
{
	raise_softirq(SCHEDULE_SOFTIRQ);
}

struct scheduler sched_agency;

void sched_agency_init(void)
{
	sched_flip.sched_data.current_dom = 0;

	spin_lock_init(&sched_agency.sched_data.schedule_lock);

	init_timer(&sched_agency.sched_data.s_timer, NULL, NULL, AGENCY_CPU);
}

struct scheduler sched_agency = {
	.name = "SOO AVZ agency activation",

	.init = sched_agency_init,

	.do_schedule = agency_schedule,
	.wake = agency_wake,
};

/* Initialise the data structures. */
void domain_scheduler_init(void)
{
	per_cpu(current_domain, AGENCY_CPU) = NULL;

	spin_lock_init(&schedule_lock);

	register_softirq(SCHEDULE_SOFTIRQ, domain_schedule);

	sched_agency.init();
	sched_flip.init();
}
