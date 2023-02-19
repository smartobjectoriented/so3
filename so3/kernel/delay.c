/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 *
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

#include <delay.h>
#include <schedule.h>
#include <timer.h>
#include <softirq.h>

#include <device/irq.h>

/**
 * Wait-free loop based on the jiffy_ref
 */
void udelay(u64 us) {

	u64 __delay = 0ull, target;

#warning review the way how to calculate the delay...
	target = ((us / ((u64) 1000000ull / (u64) 100))) * jiffies_ref;

	while (__delay < target) __delay++;

}

/*
 * Timer callback which will awake the thread.
 * IRQs are off.
 */
void delay_handler(void *arg) {
	tcb_t *tcb = (tcb_t *) arg;
	/*
	 * delay_handler may be called two ways differently; the first one (and more standard way)
	 * is right after an interrupt context during the softirq action processing. In this case,
	 * it is *sure* that the thread is in waiting state (issued from a previous sleep function which
	 * is set with IRQs off). The second case corresponds to a call along the msleep() path during the set_timer()
	 * initialization. In this case, the handler can be called if the deadline already expired and the thread will
	 * not be waiting on a delay.
	 */

	if (tcb->state == THREAD_STATE_WAITING) {

		/* If the thread is submitted to a waiting timeout,
		 * the value is re-adjusted here.
		 */

		tcb->timeout = tcb->timeout - NOW();

		ready(tcb);

		/* Trigger a schedule to give a change to the waiter */
		raise_softirq(SCHEDULE_SOFTIRQ);
	}
}

static void __sleep(u64 ns) {
	struct timer __timer;
	unsigned long flags;

	flags = local_irq_save();

	/* Create a specific timer attached to this thread */
	init_timer(&__timer, delay_handler, current(), smp_processor_id());

	current()->timeout = NOW() + ns;
	set_timer(&__timer, current()->timeout);

	/* Put the thread in waiting state *only* if the timer still makes sense. */
	if (__timer.status == TIMER_STATUS_in_list) {

		waiting();

		/* We are resumed, but not necessarly by the timer handler (in case of a semaphore timeout based synchronization
		 * mechanism, we might get the lock *before* the timeout.
		 * In this case, we have to clean the timer.
		 */
		stop_timer(&__timer);

	}

	local_irq_restore(flags);

}

/*
 * Suspend the current thread during <ms> milliseconds.
 */
void msleep(uint32_t ms) {
	__sleep(MILLISECS(ms));
}

/*
 * Suspend the current thread during <us> microseconds.
 */
void usleep(u64 us) {
	__sleep(MICROSECS(us));
}

void sleep(u64 ns) {
	__sleep(ns);
}

int do_nanosleep(const struct timespec *req, struct timespec *rem) {

	if (req->tv_nsec != 0)
		__sleep(req->tv_nsec);
	else if (req->tv_sec != 0)
		__sleep(SECONDS(req->tv_sec));

	return 0;
}

