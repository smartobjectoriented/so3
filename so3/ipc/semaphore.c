/*
 * Copyright (C) 2020 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <types.h>
#include <semaphore.h>
#include <string.h>

/*
 * Sempahore down operation - Prepare to enter a critical section
 * by means of the semaphore paradigm.
 */
void sem_down(sem_t *sem) {
	queue_thread_t q_tcb;

	for (;;) {

		mutex_lock(&sem->lock);

		if (sem->val <= 0) {
			q_tcb.tcb = current();
			/*
			 * We only attempt the xchg if the count is non-negative in order
			 * to avoid unnecessary xchg operations.
			 */
			if ((atomic_read(&sem->count) >= 0) && (atomic_xchg(&sem->count, -1) == 1))
				break;

			/* Add waiting tasks to the end of the waitqueue (FIFO) */
			q_tcb.tcb = current();
			list_add_tail(&q_tcb.list, &sem->tcb_list);

			mutex_unlock(&sem->lock);

			waiting();

		} else {
			atomic_set(&sem->count, 0);

			sem->val--;
			mutex_unlock(&sem->lock);
			break;
		}
	}

}

void sem_up(sem_t *sem) {
	queue_thread_t *curr;
	bool need_resched = false;

	mutex_lock(&sem->lock);
	sem->val++;

	atomic_set(&sem->count, 1);

	if (!list_empty(&sem->tcb_list)) {
		/* Get the waiting the first entry of this associated waitqueue */
		curr = list_first_entry(&sem->tcb_list, queue_thread_t, list);
		need_resched = true;

		list_del(&curr->list);

		wake_up(curr->tcb);

	}
	mutex_unlock(&sem->lock);

	if (need_resched)
		schedule();
}

/*
 * Must be called for every new semaphore.
 */
void sem_init(sem_t *sem) {

	memset(sem, 0, sizeof(sem_t));

	INIT_LIST_HEAD(&sem->tcb_list);
	atomic_set(&sem->count, 1);

	/* Initial value of the semaphore */
	sem->val = 1;

	mutex_init(&sem->lock);

}
