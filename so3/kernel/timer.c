/*
 * Copyright (C) 2014-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017 Baptiste Delporte <bonel@bonel.net>
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

#include <types.h>
#include <spinlock.h>
#include <softirq.h>
#include <delay.h>
#include <schedule.h>
#include <errno.h>
#include <smp.h>
#include <timer.h>
#include <percpu.h>
#include <heap.h>

#include <avz/keyhandler.h>

#include <asm/backtrace.h>
#include <asm/div64.h>

#include <device/timer.h>
#include <device/irq.h>

#warning set_errno
#define set_errno(x)

struct timers {

	spinlock_t lock;
	struct timer **heap;
	struct timer *list;
	struct timer *running;

}__cacheline_aligned;

static DEFINE_PER_CPU(struct timers, timers);

static void remove_from_list(struct timer **list, struct timer *t) {
	struct timer *curr, *prev;

	if (*list == t) {
		*list = t->list_next;
		t->list_next = NULL;
	} else {

		curr = *list;
		while (curr != t) {
			prev = curr;
			curr = curr->list_next;
		}
		prev->list_next = curr->list_next;
		curr->list_next = NULL;
	}
}

static void add_to_list(struct timer **list, struct timer *t) {
	struct timer *curr;

	if (*list == NULL) {
		*list = t;
		t->list_next = NULL;
	} else {

		/* Check for an existing timer and update the deadline if so. */

		curr = *list;
		while (curr != NULL) {
			if (curr == t) {
				curr->expires = t->expires;
				return;
			}
			curr = curr->list_next;
		}

		/* Not found, we add the new timer at the list head */
		t->list_next = *list;
		*list = t;

	}
}

/**
 * clocks_calc_mult_shift - calculate mult/shift factors for scaled math of clocks
 * @mult:	pointer to mult variable
 * @shift:	pointer to shift variable
 * @from:	frequency to convert from
 * @to:		frequency to convert to
 * @maxsec:	guaranteed runtime conversion range in seconds
 *
 * The function evaluates the shift/mult pair for the scaled math
 * operations of clocksources and clockevents.
 *
 * @to and @from are frequency values in HZ. For clock sources @to is
 * NSEC_PER_SEC == 1GHz and @from is the counter frequency. For clock
 * event @to is the counter frequency and @from is NSEC_PER_SEC.
 *
 * The @maxsec conversion range argument controls the time frame in
 * seconds which must be covered by the runtime conversion with the
 * calculated mult and shift factors. This guarantees that no 64bit
 * overflow happens when the input value of the conversion is
 * multiplied with the calculated mult factor. Larger ranges may
 * reduce the conversion accuracy by chosing smaller mult and shift
 * factors.
 */
void clocks_calc_mult_shift(u32 *mult, u32 *shift, u32 from, u32 to, u32 maxsec) {
	u64 tmp;
	u32 sft, sftacc = 32;

	/*
	 * Calculate the shift factor which is limiting the conversion
	 * range:
	 */
	tmp = ((u64) maxsec * from) >> 32;
	while (tmp) {
		tmp >>= 1;
		sftacc--;
	}

	/*
	 * Find the conversion shift/mult pair which has the best
	 * accuracy and fits the maxsec conversion range:
	 */
	for (sft = 32; sft > 0; sft--) {
		tmp = (u64) to << sft;
		tmp += from / 2;
		do_div(tmp, from);
		if ((tmp >> sftacc) == 0)
			break;
	}
	*mult = tmp;
	*shift = sft;
}

static int remove_entry(struct timers *timers, struct timer *t) {
	int rc = 0;

	switch (t->status) {
	case TIMER_STATUS_in_list:
		remove_from_list(&timers->list, t);
		break;

	default:
		rc = 0;
		printk("t->status = %d\n", t->status);
		dump_stack();
		BUG();
	}

	t->status = TIMER_STATUS_inactive;
	return rc;
}

static void add_entry(struct timers *timers, struct timer *t) {

	ASSERT(t->status == TIMER_STATUS_inactive);

	t->status = TIMER_STATUS_in_list;

	add_to_list(&timers->list, t);
}

static inline void add_timer(struct timer *timer) {
	add_entry(&per_cpu(timers, timer->cpu), timer);
}

static inline void timer_lock(struct timer *timer) {
	spin_lock(&per_cpu(timers, timer->cpu).lock);
}

static inline void timer_unlock(struct timer *timer) {
	spin_unlock(&per_cpu(timers, timer->cpu).lock);
}

/*
 * Stop a timer, i.e. remove from the timer list.
 */
void __stop_timer(struct timer *timer) {

	if (active_timer(timer))
		remove_entry(&per_cpu(timers, timer->cpu), timer);
}

void set_timer(struct timer *timer, u64 expires) {

	timer_lock(timer);

	/* Must have been initialized */
	BUG_ON(timer->function == NULL);

	if (active_timer(timer))
		__stop_timer(timer);

	timer->expires = expires;

	if (likely(timer->status != TIMER_STATUS_killed))
		add_timer(timer);

	timer_unlock(timer);

	/* Since we add a new timer, the deadline can be earlier than the current
	 * programmed timer, so a re-programming might be necessary.
	 */

	raise_softirq(TIMER_SOFTIRQ);

	/* Make sure than a possible call to schedule() can be performed */
	if (!__in_interrupt)
		do_softirq();

}

void stop_timer(struct timer *timer) {

	timer_lock(timer);
	__stop_timer(timer);
	timer_unlock(timer);
}

void kill_timer(struct timer *timer) {

	BUG_ON(this_cpu(timers).running == timer);

	timer_lock(timer);

	if (active_timer(timer))
		__stop_timer(timer);

	timer->status = TIMER_STATUS_killed;

	timer_unlock(timer);
}

static void execute_timer(struct timers *ts, struct timer *t) {
	void (*fn)(void *) = t->function;
	void *data = t->data;

	ts->running = t;

	spin_unlock(&ts->lock);
	(*fn)(data);
	spin_lock(&ts->lock);

	ts->running = NULL;

}

/*
 * Main timer softirq processing
 */
static void timer_softirq_action(void) {
	struct timer *cur, *t, *start;
	struct timers *ts;
	u64 now;
	u64 end = STIME_MAX;

	ts = &this_cpu(timers);

	spin_lock(&ts->lock);

	preempt_disable();

again:
	now = NOW();

	/* Execute ready list timers. */
	cur = ts->list;

	/* Verify if some timers reached their deadline, remove them and execute them if any. */
	while (cur != NULL) {
		t = cur;
		cur = cur->list_next;

		if (t->expires <= now) {

			DBG("### %s: NOW: %llu executing timer expires: %llu   ***  delta: %d\n", __func__, now, t->expires, t->expires - now);

			remove_entry(ts, t);
			execute_timer(ts, t);
		}
	}

	/* Examine the pending timers to get the earliest deadline */
	start = NULL;
	t = ts->list;
	while (t != NULL) {
		DBG("### %s: NOW: %llu pending expires: %llu   ***  delta: %d\n", __func__, now, t->expires, t->expires - now);
		if (((start == NULL) && (t->expires < end)) || (t->expires < start->expires))
			start = t;

		t = t->list_next;
	}

	if (start != NULL) {
		if (timer_dev_set_deadline(start->expires))
			goto again;

	}

	preempt_enable();

	spin_unlock(&ts->lock);
}

#ifdef CONFIG_AVZ

static void dump_timer(struct timer *t, u64 now) {
	/* We convert 1000 to u64 in order to use the well-implemented __aeabi_uldivmod function */
	lprintk("  expires = %llu, now = %llu, expires - now = %llu ns timer=%p cb=%p(%p) cpu=%d\n",
			t->expires, now, t->expires - now, t, t->function, t->data, t->cpu);
}

static void dump_timerq(unsigned char key) {
	struct timer *t;
	struct timers *ts;
	u64 now = NOW();
	int j;

	printk("Dumping timer queues:\n");

	ts = &per_cpu(timers, ME_CPU);
	printk("CPU #%d:\n", ME_CPU);

	spin_lock(&ts->lock);

	for (t = ts->list, j = 0; t != NULL; t = t->list_next, j++)
		dump_timer(t, now);

	spin_unlock(&ts->lock);
}

static struct keyhandler dump_timerq_keyhandler = {
		.fn = dump_timerq,
		.desc = "dump timer queues"
};

#endif /* CONFIG_AVZ */

/*
 * Main timer initialization
 */
void timer_init(void) {

	register_softirq(TIMER_SOFTIRQ, timer_softirq_action);

	spin_lock_init(&per_cpu(timers, smp_processor_id()).lock);
	
	/* The timer devices have been previously initialized during devices_init() */
#ifdef CONFIG_RTOS
	BUG_ON(oneshot_timer.start == NULL);
	oneshot_timer.start();
#else
	BUG_ON(periodic_timer.start == NULL);
	periodic_timer.start();
#endif /* !CONFIG_RTOS */

#ifdef CONFIG_AVZ
	register_keyhandler('a', &dump_timerq_keyhandler);
#endif
}

/*
 * This function gets the current time and put it in the parameter tv
 */
int do_get_time_of_day(struct timespec *ts)
{
        u64 time;

        if (!ts) {
                set_errno(EINVAL);
                return -1;
        }

        time = NOW();

        ts->tv_sec = time / (time_t) 1000000000;
        ts->tv_nsec = time;

        return 0;
}

/*
 *  This function retrieves the time of the specified clock clk_id.
 *
 *  <clk_id> should be equal to CLOCK_REALTIME or CLOCK_MONOTONIC.
 *
 *  Currently, we only support CLOCK_MONOTONIC since we do not manage a RTC yet.
 */
int do_get_clock_time(int clk_id, struct timespec *ts)
{
        u64 time;

        if (!ts) {
                set_errno(EINVAL);
                return -1;
        }

        time = NOW();

        ts->tv_sec = time / (time_t) 1000000000;
        ts->tv_nsec = time;

        return 0;
}

