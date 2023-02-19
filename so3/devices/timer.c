/*
 * Copyright (C) 2014-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <spinlock.h>
#include <timer.h>
#include <softirq.h>

#ifdef CONFIG_AVZ

#include <avz/domain.h>
#include <avz/vcpu.h>
#include <avz/sched.h>

#endif /* CONFIG_AVZ */

#include <device/timer.h>
#include <device/irq.h>

/* Value used to detect a potential clocksource wrap */
#define CYCLE_DELTA_MIN		0x100000000ull

static u64 sys_time = 0ull;

/* The three main timers in SO3 */

periodic_timer_t periodic_timer;
oneshot_timer_t oneshot_timer;
clocksource_timer_t clocksource_timer;

#ifdef CONFIG_AVZ

void timer_interrupt(bool periodic) {
	int i;

	if (periodic) {

		/* Now check for ticking the non-realtime domains which need periodic ticks. */
		for (i = 2; i < MAX_DOMAINS; i++) {
			/*
			 * We have to check if the domain exists and its VCPU has been created. If not,
			 * there is no need to propagate the timer event.
			 */
			if ((domains[i] != NULL) && !domains[i]->is_dying) {
				if ((domains[i]->runstate == RUNSTATE_running) || (domains[i]->runstate == RUNSTATE_runnable)) {
					if (domains[i]->need_periodic_timer)

						/* Forward to the guest */
						send_timer_event(domains[i]);
				}
			}
		}
	}

	 /* Raise a softirq on the CPU which is processing the interrupt. */
	raise_softirq(TIMER_SOFTIRQ);
}

#endif /* CONFIG_AVZ */

/*
 * Return the time in ns from the monotonic clocksource.
 */
u64 get_s_time(void) {
	u64 cycle_now, cycle_delta;
	unsigned long flags;

	/* Protect against concurrent access from different CPUs */

	flags = local_irq_save();

	if (!clocksource_timer.read)
		return 0;

	cycle_now = clocksource_timer.read();

	/*
	 * cycle_last can be greater than cycle_now in two cases:
	 * - The clocksource counter has wrapped
	 * - There is an issue due to the hardware clocksource, due to jittering for instance.
	 * The first case is handled using the mask.
	 * We have to pay attention to and handle the second case to avoid too big values for cycle_delta due to the small interval
	 * between cycle_now and cycle_last.
	 */
	if (unlikely((cycle_now < clocksource_timer.cycle_last) && (clocksource_timer.cycle_last - cycle_now < CYCLE_DELTA_MIN))) {
		cycle_now = clocksource_timer.cycle_last;
		cycle_delta = 0;
	} else
		cycle_delta = (cycle_now - clocksource_timer.cycle_last) & clocksource_timer.mask;

	clocksource_timer.cycle_last = cycle_now;

	sys_time += cyc2ns(cycle_delta);

	local_irq_restore(flags);

	return sys_time;
}

/*
 * repogram_timer - May be used from various CPUs
 *
 * deadline is the expiring time expressed in ns.
 * Returns true if the deadline already expired (in the meanwhile for instance).
 *
 */
bool timer_dev_set_deadline(u64 deadline) {
	int64_t delta;

	/* Only the oneshot timer will be possibly reprogrammed */

	delta = deadline - NOW();

	if (delta <= 0) {

		raise_softirq(TIMER_SOFTIRQ);
		if (!__in_interrupt)
			return true;

	} else if (oneshot_timer.set_delay != NULL)
		oneshot_timer.set_delay(delta);

	return false;
}


void timer_dev_init(void) {

	memset(&periodic_timer, 0, sizeof(periodic_timer_t));
	memset(&oneshot_timer, 0, sizeof(oneshot_timer_t));
	memset(&clocksource_timer, 0, sizeof(clocksource_timer_t));

}

#ifdef CONFIG_AVZ

extern void send_guest_virq(struct domain *d, int virq);

void send_timer_event(struct domain *d) {
	send_guest_virq(d, VIRQ_TIMER);
}

#endif /* CONFIG_AVZ */
