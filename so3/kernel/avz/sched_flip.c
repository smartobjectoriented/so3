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

#if 0
#define DEBUG
#endif

#include <common.h>
#include <timer.h>
#include <softirq.h>
#include <errno.h>

#include <avz/sched.h>
#include <avz/sched-if.h>
#include <avz/debug.h>
#include <avz/domctl.h>

DEFINE_SPINLOCK(schedflip_lock);

extern spinlock_t softirq_pending_lock;

static struct domain *domains_runnable[MAX_DOMAINS];
struct scheduler sched_flip;

/*
 * Return the number of runnable domains for this scheduler policy.
 */
static int flip_quant_runnable(void) {
	int i = 0;
	int nr = 0;

	for (i = 0; i < MAX_DOMAINS; i++)
		if (domains_runnable[i] != NULL)
			nr++;

	return nr;
}

/*
 * Main scheduling function
 * Reasons for calling this function are:
 * -timeslice ended up
 * -IRQ in upcall path towards the agency guest
 */
struct task_slice flip_do_schedule(void)
{
	struct task_slice ret;
	unsigned int loopmax = MAX_DOMAINS;
	struct schedule_data *sd;

	ret.d = NULL;

	sd = &sched_flip.sched_data;

	/* Switch can also occur if time slice expired */

	do {
		sd->current_dom = (sd->current_dom + 1) % MAX_DOMAINS;
		ret.d = domains_runnable[sd->current_dom];

		loopmax--;
	} while ((ret.d == NULL) && loopmax);

	if (ret.d == NULL)
		ret.d = idle_domain[smp_processor_id()];

	if (flip_quant_runnable() <= 1)
		ret.time = 0;  /* Keep the schedule_softirq timer disabled */
	else
		ret.time = CONFIG_SCHED_FLIP_SCHEDFREQ;

#if 0
	printk("%s: on cpu %d picking now: %lx \n", __func__, smp_processor_id(), ret.d->avz_shared->domID);
#endif

	return ret;
}

/*
 * schedule_lock is acquired.
 */
static void flip_sleep(struct domain *d)
{
	DBG("flip_sleep was called, domain-id %i\n", d->avz_shared->domID);

	if (is_idle_domain(d))
		return;

	domains_runnable[d->avz_shared->domID] = NULL;

	if (sched_flip.sched_data.current_dom == d->avz_shared->domID)
		cpu_raise_softirq(d->processor, SCHEDULE_SOFTIRQ);

}


static void flip_wake(struct domain *d)
{
	DBG("flip_wake was called, domain-id %i\n", d->avz_shared->domID);

	domains_runnable[d->avz_shared->domID] = d;

	cpu_raise_softirq(d->processor, SCHEDULE_SOFTIRQ);

	/* We do not manage domain priorities, so we do not invoke schedule() at this time */

}

#ifdef CONFIG_SOO

/* The scheduler timer: force a run through the scheduler */
static void s_timer_fn(void *unused)
{
	raise_softirq(SCHEDULE_SOFTIRQ);
}

#endif

void sched_flip_init(void) {
	int i;

	for (i = 0; i < MAX_DOMAINS; i++)
		domains_runnable[i] = NULL;

	sched_flip.sched_data.current_dom = 0;

	spin_lock_init(&sched_flip.sched_data.schedule_lock);

#ifdef CONFIG_SOO
	/* Initiate a timer to trigger the schedule function */
	init_timer(&sched_flip.sched_data.s_timer, s_timer_fn, NULL, ME_CPU);

	set_timer(&sched_flip.sched_data.s_timer, NOW() + MILLISECS(CONFIG_SCHED_FLIP_SCHEDFREQ));
#endif /* CONFIG_SOO */

}

struct scheduler sched_flip = {
	.name = "SOO AVZ flip scheduler",

	.init = sched_flip_init,

	.do_schedule = flip_do_schedule,

	.sleep = flip_sleep,
	.wake = flip_wake,
};
