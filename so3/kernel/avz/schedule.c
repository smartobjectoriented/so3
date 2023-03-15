/*
 * Copyright (C) 2014-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <errno.h>

#include <device/irq.h>

#include <avz/event.h>
#include <avz/sched.h>
#include <avz/sched-if.h>
#include <avz/domain.h>

DEFINE_PER_CPU(struct domain *, current_domain);

static spinlock_t schedule_lock;

/* Various timer handlers. */

inline void domain_runstate_change(struct domain *d, int new_state) {

	/*
	 * We might already be in RUNSTATE_blocked before setting to this state; for example,
	 * if a ME has been paused and migrates, and is killed during the cooperation phase,
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
	bool __already_locked = false;

	if (spin_is_locked(&d->sched->sched_data.schedule_lock))
		__already_locked = true;
	else
		spin_lock(&d->sched->sched_data.schedule_lock);

	if (d->runstate >= RUNSTATE_blocked) {
		domain_runstate_change(d, RUNSTATE_runnable);

		d->sched->wake(d);

		clear_bit(_VPF_blocked, &d->pause_flags);
	}

	if (!__already_locked)
		spin_unlock(&d->sched->sched_data.schedule_lock);

}

void sched_destroy_domain(struct domain *d)
{
	kill_timer(&d->oneshot_timer);
}

void vcpu_sleep_nosync(struct domain *d)
{
	bool __already_locked = false;

	if (spin_is_locked(&d->sched->sched_data.schedule_lock))
		__already_locked = true;
	else
		spin_lock(&d->sched->sched_data.schedule_lock);

	/*
	 * Stop associated timers.
	 * This code is executed by CPU #0. It means that we have to take care about potential
	 * re-insertion of new timers by CPU #3 activities after the oneshot_timer has been stopped.
	 * For this reason, we first set the VPF_blocked bit to 1, and the sched_deadline/sched_sleep have
	 * to check for this bit.
	 */
	set_bit(_VPF_blocked, &d->pause_flags);

	/* Now, setting the domain to blocked state */
	if (d->runstate != RUNSTATE_running)
		domain_runstate_change(d, RUNSTATE_blocked);

	if (active_timer(&d->oneshot_timer))
		stop_timer(&d->oneshot_timer);

	d->sched->sleep(d);

	if (!__already_locked)
		spin_unlock(&d->sched->sched_data.schedule_lock);
}

/*
 * Set a domain sleeping. The domain state is set to blocked.
 */
void vcpu_sleep_sync(struct domain *d)
{
	vcpu_sleep_nosync(d);

	while (d->is_running)
		cpu_relax();

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
	struct domain        *prev = current_domain, *next = NULL;
	struct schedule_data *sd;
	struct task_slice     next_slice;

	ASSERT(local_irq_is_disabled());

	ASSERT(prev->runstate == RUNSTATE_running);

	/* To avoid that another CPU manipulates scheduler data structures */
	/* Maybe the lock is already acquired from do_sleep() for example */
	if (!spin_is_locked(&current_domain->sched->sched_data.schedule_lock))
		spin_lock(&current_domain->sched->sched_data.schedule_lock);

	sd = &current_domain->sched->sched_data;

	stop_timer(&sd->s_timer);

	/* get policy-specific decision on scheduling... */
	next_slice = prev->sched->do_schedule();

	next = next_slice.d;

#ifdef CONFIG_SOO
	if (next_slice.time > 0ull)
		set_timer(&next->sched->sched_data.s_timer, NOW() + MILLISECS(next_slice.time));
#endif /* CONFIG_SOO */

	if (unlikely(prev == next))
	{
		spin_unlock(&prev->sched->sched_data.schedule_lock);
		ASSERT(prev->runstate == RUNSTATE_running);

		return ;
	}
	ASSERT(prev->runstate == RUNSTATE_running);

	domain_runstate_change(prev, (test_bit(_VPF_blocked, &prev->pause_flags) ? RUNSTATE_blocked : (prev->is_dying ? RUNSTATE_offline : RUNSTATE_runnable)));

	ASSERT(next->runstate != RUNSTATE_running);
	domain_runstate_change(next, RUNSTATE_running);

	ASSERT(!next->is_running);
	next->is_running = 1;

#if 0
	printk("### running on cpu: %d prev: %d next: %d\n", smp_processor_id(), prev->avz_shared->domID, next->avz_shared->domID);
#endif

	/* We do not unlock the schedulder_lock until everything has been processed */

	context_switch(prev, next);

	/* From here, prev and next are those in the current domain; don't forget ;-) */

}

/** Just to bootstrap the agency **/
static struct task_slice agency_schedule(void)
{
	struct task_slice ts;

	ts.d = domains[0];
	ts.time = 0;

	return ts;
}

static void agency_wake(struct domain *d) {
	raise_softirq(SCHEDULE_SOFTIRQ);
}

struct scheduler sched_agency = {
	.name = "SOO AVZ agency activation",

	.wake = agency_wake,
	.do_schedule = agency_schedule
};


/* Initialise the data structures. */
void  domain_scheduler_init(void)
{
	per_cpu(current_domain, AGENCY_CPU) = NULL;

	spin_lock_init(&schedule_lock);

	register_softirq(SCHEDULE_SOFTIRQ, domain_schedule);

	sched_flip.init();
}

