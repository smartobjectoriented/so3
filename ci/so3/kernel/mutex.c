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

/*
 *
 * Locking functions for SO3  - inspired from Linux code, however quite different
 *
 */

#include <mutex.h>
#include <schedule.h>
#include <string.h>
#include <process.h>

void mutex_lock(struct mutex *lock) {
	unsigned long flags;
	queue_thread_t q_tcb;

	/*
	 * We get a spinlock with IRQs off after acquiring to avoid a race condition which
	 * may happen if a user signal handler is executed and does some syscall processing
	 * which could require the same kernel locks than the current thread.
	 */
	flags = spin_lock_irqsave(&lock->wait_lock);

	/*
	 * Once more, try to acquire the lock. Only try-lock the mutex if
	 * it is unlocked to reduce unnecessary xchg() operations.
	 */
	if (!mutex_is_locked(lock) && (atomic_xchg(&lock->count, 0) == 1))
		goto skip_wait;

	if (mutex_is_locked(lock) && (lock->owner == current())) {
		lock->recursive_count++;
		goto skip_wait;
	}

	for (;;) {
		/*
		 * Lets try to take the lock again - this is needed even if
		 * we get here for the first time (shortly after failing to
		 * acquire the lock), to make sure that we get a wakeup once
		 * it's unlocked. Later on, if we sleep, this is the
		 * operation that gives us the lock. We xchg it to -1, so
		 * that when we release the lock, we properly wake up the
		 * other waiters.
		 *
		 * We only attempt the xchg if the count is non-negative in order
		 * to avoid unnecessary xchg operations.
		 */
		if ((atomic_read(&lock->count) >= 0) && (atomic_xchg(&lock->count, -1) == 1))
			break;

		/* Add waiting tasks to the end of the waitqueue (FIFO) */
		q_tcb.tcb = current();
		list_add_tail(&q_tcb.list, &lock->tcb_list);

		/* didn't get the lock, go to sleep. */
		spin_unlock(&lock->wait_lock);

		waiting();

		BUG_ON(local_irq_is_enabled());

		/* Since IRQs are off anyway, we are sure to re-acquire the lock (another thread could
		 * try to acquire this mutex right after the other thread wakes up, but in this case it will be
		 * blocked wince we have not finished unlocking (lock->count != 0).
		 */
		spin_lock(&lock->wait_lock);
	}

	/* set it to 0 if there are no waiters left */
	if (likely(list_empty(&lock->tcb_list)))
		atomic_set(&lock->count, 0);

skip_wait:

	/* got the lock - cleanup and rejoice! */
	lock->owner = current();

	spin_unlock_irqrestore(&lock->wait_lock, flags);

}

void mutex_unlock(struct mutex *lock) {

	queue_thread_t *curr;
	bool need_resched = false;
	unsigned long flags;

	BUG_ON(!mutex_is_locked(lock));

	flags = spin_lock_irqsave(&lock->wait_lock);

	if (lock->recursive_count) {
		lock->recursive_count--;
		spin_unlock_irqrestore(&lock->wait_lock, flags);
		return ;
	}
	spin_unlock_irqrestore(&lock->wait_lock, flags);

	/*
	 * As a performance measurement, release the lock before doing other
	 * wakeup related duties to follow. This allows other tasks to acquire
	 * the lock sooner, while still handling cleanups in past unlock calls.
	 * This can be done as we do not enforce strict equivalence between the
	 * mutex counter and wait_list.
	 *
	 * Some architectures leave the lock unlocked in the fastpath failure
	 * case, others need to leave it locked. In the later case we have to
	 * unlock it here - as the lock counter is currently 0 or negative.
	 */

	atomic_set(&lock->count, 1);

	flags = spin_lock_irqsave(&lock->wait_lock);

	if (!list_empty(&lock->tcb_list)) {

		/* Get the waiting the first entry of this associated waitqueue */
		curr = list_first_entry(&lock->tcb_list, queue_thread_t, list);
		need_resched = true;

		list_del(&curr->list);

		ready(curr->tcb);

	}

	spin_unlock_irqrestore(&lock->wait_lock, flags);
	if (need_resched)
		schedule();
}

/*
 * The following syscall implementation are a first attempt, mainly used for debugging kernel mutexes.
 */
int do_mutex_lock(mutex_t *lock) {
	mutex_lock(lock);

	return 0;
}

int do_mutex_unlock(mutex_t *lock) {
	mutex_unlock(lock);

	return 0;
}

void mutex_init(struct mutex *lock) {

	memset(lock, 0, sizeof(struct mutex));

	INIT_LIST_HEAD(&lock->tcb_list);
	atomic_set(&lock->count, 1);
	spin_lock_init(&lock->wait_lock);
}




