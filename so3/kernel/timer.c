/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <div64.h>
#include <delay.h>
#include <schedule.h>
#include <errno.h>

#include <device/timer.h>
#include <device/irq.h>

static void timer_softirq_action(void);

struct timers {
	struct timer *list;
	struct timer *running;

}__cacheline_aligned;

static struct timers timers;

static void remove_from_list(struct timer *t) {
	struct timer *curr, *prev;

	if (timers.list == t) {
		timers.list = t->list_next;
		t->list_next = NULL;
	} else {

		curr = timers.list;
		while (curr != t) {
			prev = curr;
			curr = curr->list_next;
		}
		prev->list_next = curr->list_next;
		curr->list_next = NULL;
	}
}

static void add_to_list(struct timer *t) {
	struct timer *curr;

	if (timers.list == NULL) {
		timers.list = t;
		t->list_next = NULL;
	} else {

		/* Check for an existing timer and update the deadline if so. */

		curr = timers.list;
		while (curr != NULL) {
			if (curr == t) {
				curr->expires = t->expires;
				return ;
			}
			curr = curr->list_next;
		}

		/* Not found, we add the new timer at the list head */
		t->list_next = timers.list;
		timers.list = t;

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

static int remove_entry(struct timer *t) {
	int rc = 0;

	switch (t->status) {
	case TIMER_STATUS_in_list:
		remove_from_list(t);
		break;

	default:
		rc = 0;
		printk("t->status = %d\n", t->status);
		BUG();
	}


	t->status = TIMER_STATUS_inactive;
	return rc;
}

static void add_entry(struct timer *t) {

	ASSERT(t->status == TIMER_STATUS_inactive);

	t->status = TIMER_STATUS_in_list;

	add_to_list(t);
}

static inline void add_timer(struct timer *timer) {
	add_entry(timer);
}


/*
 * Stop a timer, i.e. remove from the timer list.
 * If a timer is not attached to a timer-capable CPU, a sibling timer for oneshot timer (non periodic) has been allocated.
 * In this case, we need to stop it (if active) and de-allocate it.
 */
void __stop_timer(struct timer *timer) {

	if (active_timer(timer))
		remove_entry(timer);
}

void set_timer(struct timer *timer, u64 expires) {
	uint32_t flags;

	flags = local_irq_save();

	/* Must have been initialized */
	BUG_ON(timer->function == NULL);

	if (active_timer(timer))
		__stop_timer(timer);

	timer->expires = expires;

	if (likely(timer->status != TIMER_STATUS_killed))
		add_timer(timer);

	/* Since we add a new timer, the deadline can be earlier than the current
	 * programmed timer, so a re-programming might be necessary.
	 */

	raise_softirq(TIMER_SOFTIRQ);

	/* Make sure than a possible call to schedule() can be performed */
	if (!__in_interrupt)
		do_softirq();

	local_irq_restore(flags);
}

void stop_timer(struct timer *timer) {

	uint32_t flags;

	flags = local_irq_save();

	__stop_timer(timer);

	local_irq_restore(flags);
}

void kill_timer(struct timer *timer) {

	uint32_t flags;

	BUG_ON(timers.running == timer);

	flags = local_irq_save();

	if (active_timer(timer))
		__stop_timer(timer);

	timer->status = TIMER_STATUS_killed;

	local_irq_restore(flags);
}

/*
 * execute_timer() is called when a timer deadline is reached.
 */
static void execute_timer(struct timer *t) {
	uint32_t flags;
	void (*fn)(void *) = t->function;
	void *data = t->data;

	flags = local_irq_save();

	timers.running = t;

	BUG_ON(fn == NULL);

	(*fn)(data);

	timers.running = NULL;

	local_irq_restore(flags);
}

/*
 * Main timer softirq processing
 */
static void timer_softirq_action(void) {
	struct timer *cur, *t, *start;
	u64 now;
	u64 end = STIME_MAX;
	uint32_t flags;

	flags = local_irq_save();

	preempt_disable();
again:
	now = NOW();

	/* Execute ready list timers. */
	cur = timers.list;

	/* Verify if some timers reached their deadline, remove them and execute them if any. */

	while (cur != NULL) {
		t = cur;
		cur = cur->list_next;

		if (t->expires <= now) {

			DBG("### %s: NOW: %llu executing timer expires: %llu   ***  delta: %d\n", __func__, now, t->expires, t->expires - now);

			remove_entry(t);
			execute_timer(t);
		}
	}

	/* Examine the pending timers to get the earliest deadline */
	start = NULL;
	t = timers.list;
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

	local_irq_restore(flags);

}

static void dump_timer(struct timer *t, u64 now) {
	/* We convert 1000 to u64 in order to use the well-implemented __aeabi_uldivmod function */
	lprintk("  expires = %llu, now = %llu, expires - now = %llu ns timer=%p cb=%p(%p)\n",
			t->expires, now, t->expires - now, t, t->function, t->data);
}

void dump_timers(void) {
	struct timer *t;
	u64 now = NOW();
	int j;
	uint32_t flags;

	lprintk("Dumping timer queues:\n");

	flags = local_irq_save();

	for (t = timers.list, j = 0; t != NULL; t = t->list_next, j++)
		dump_timer(t, now);

	local_irq_restore(flags);
}


/*
 * Main timer initialization
 */
void timer_init(void) {

	register_softirq(TIMER_SOFTIRQ, timer_softirq_action);

	/* The timer devices have been previously initialized during devices_init() */
#ifdef CONFIG_RTOS
	BUG_ON(oneshot_timer.start == NULL);
	oneshot_timer.start();
#else
	BUG_ON(periodic_timer.start == NULL);
	periodic_timer.start();
#endif /* !CONFIG_RTOS */

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

        ts->tv_sec = time / 1000000000ull;
        ts->tv_nsec = (long) time;

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
        ts->tv_sec = time / 1000000000ull;
        ts->tv_nsec = (long) time;

        return 0;
}

