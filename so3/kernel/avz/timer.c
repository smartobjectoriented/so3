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

#include <avz/domain.h>
#include <avz/vcpu.h>
#include <avz/sched.h>

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

extern void send_guest_virq(struct domain *d, int virq);

void send_timer_event(struct domain *d) {
	send_guest_virq(d, VIRQ_TIMER);
}

