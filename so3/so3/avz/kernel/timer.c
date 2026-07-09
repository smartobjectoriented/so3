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

#include <spinlock.h>
#include <timer.h>
#include <softirq.h>

#include <avz/domain.h>
#include <avz/sched.h>

/*
 * Called on every EL2 timer tick (CNTHP, PPI 26) on agency CPUs (non-capsule).
 * Per-CPU tick counter so each CPU emits its own 5s heartbeat — without
 * this, CPU0's heartbeat would mask whether secondary CPUs are ticking.
 */
static DEFINE_PER_CPU(unsigned int, agency_tick_count);

/* EDGEMTech instrumentation: per-CPU counters to discriminate stall cause.
 * Updated from gic_handle/gic_inject_irq (gic.c). */
DEFINE_PER_CPU(unsigned long, gic_iar_count);
DEFINE_PER_CPU(unsigned long, gic_inj_27_count);
DEFINE_PER_CPU(unsigned long, gic_inj_total_count);
DEFINE_PER_CPU(unsigned long, gic_busy_count);
DEFINE_PER_CPU(unsigned long, gic_eexist_count);
DEFINE_PER_CPU(unsigned long, gic_sgi0_recv);
DEFINE_PER_CPU(unsigned long, gic_sgi0_eexist);

void agency_timer_interrupt(void)
{
	unsigned int *count = &this_cpu(agency_tick_count);

	if (++(*count) >= 5 * CONFIG_HZ) {
		*count = 0;

#if 0 /* Debug purpose */
		printk("[AVZ] alive on CPU%d\n", smp_processor_id());
#endif /* 0 */
	}
}

void timer_interrupt(bool periodic)
{
	int i;

	if (periodic) {
		/* Now check for ticking the guest containers which need periodic ticks. */
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
	} else {
		agency_timer_interrupt();
	}

	raise_softirq(TIMER_SOFTIRQ);
}

extern void send_guest_virq(struct domain *d, int virq);

void send_timer_event(struct domain *d)
{
	/* Do not pile up notifications on a domain which has not consumed the
	 * previous one yet (e.g. a freshly resumed capsule with interrupts
	 * still masked in early resume_fn): re-raising the event only
	 * generates a useless self-SGI per tick on the capsule CPU, and under
	 * emulation that per-tick overhead can exceed the tick period so the
	 * guest never gets to run again (livelock). A single pending upcall
	 * is enough — it is delivered as soon as the domain runs. */
	if (d->avz_shared->evtchn_upcall_pending)
		return;

	send_guest_virq(d, VIRQ_TIMER);
}
