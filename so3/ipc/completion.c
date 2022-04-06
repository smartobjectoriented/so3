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

#include <completion.h>
#include <schedule.h>
#include <spinlock.h>
#include <softirq.h>
#include <string.h>

#include <device/irq.h>

/*
 * Wait for completion function.
 * IRQs are disabled to maintain a coherent state, especially along the waiting path (no schedule are allowed).
 * This function can *not* be called from an interrupt context.
 */
void wait_for_completion(completion_t *completion) {
	queue_thread_t q_tcb;
	unsigned long flags;

	ASSERT(!__in_interrupt);
	ASSERT(local_irq_is_enabled());

	flags = local_irq_save();

	q_tcb.tcb = current();

	if (!completion->count) {
		list_add_tail(&q_tcb.list, &completion->tcb_list);

		while (!completion->count)
			waiting();

	}
	completion->count--;

	local_irq_restore(flags);
}

/*
 * Wake a thread waiting on a completion.
 * IRQs are disabled; this function can be safely called from an interrupt context.
 */
void complete(completion_t *completion) {
	queue_thread_t *curr;
	unsigned long flags;

	flags = local_irq_save();

	completion->count++;

	if (!list_empty(&completion->tcb_list)) {
		curr = list_first_entry(&completion->tcb_list, queue_thread_t, list);
		ready(curr->tcb);
		list_del(&curr->list);
	}

	/* Trigger a schedule to give a change to the waiter */
	raise_softirq(SCHEDULE_SOFTIRQ);

	local_irq_restore(flags);

}

/*
 * Wake all threads waiting for a completion.
 * In this implementation, all waiting threads are woken up and the completion counter is reset to 0
 * so that a next call to wait_for_completion() will suspend the thread on this completion.
 * IRQs are disabled; this function can be safely called from an interrupt context.
 */
void complete_all(completion_t *completion) {
	queue_thread_t *curr, *tmp;
	unsigned long flags;

	flags = local_irq_save();

	list_for_each_entry_safe(curr, tmp, &completion->tcb_list, list) {
		ready(curr->tcb);
		list_del(&curr->list);
	}

	reinit_completion(completion);

	/* Trigger a schedule to give a change to the waiter */
	raise_softirq(SCHEDULE_SOFTIRQ);

	local_irq_restore(flags);

}

void init_completion(completion_t *completion) {

	memset(completion, 0, sizeof(completion_t));

	INIT_LIST_HEAD(&completion->tcb_list);

	completion->count = 0;
}
