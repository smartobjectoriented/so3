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
	uint32_t flags;
	struct mutex_waiter waiter;

	/*
	 * We get a spinlock with IRQs off after acquiring to avoid a race condition which
	 * may happen if a user signal handler is executed and does some syscall processing
	 * which could require the same kernel locks than the current thread.
	 */
	spin_lock_irqsave(&lock->wait_lock, flags);

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

	memset(&waiter, 0, sizeof(waiter));

	for (;;) {
		/*
		 * Lets try to take the lock again - this is needed even if
		 * we get here for the first time (shortly after failing to
		 * acquire the lock), to make sure that we get a wakeup once
		 * it's unlocked. Later on, if we sleep, this is the
		 * operation that gives us the lock. We xchg it to -1, so
		 * that when we release the lock, we properly wake up the
		 * other waiters. We only attempt the xchg if the count is
		 * non-negative in order to avoid unnecessary xchg operations:
		 */
		if ((atomic_read(&lock->count) >= 0) && (atomic_xchg(&lock->count, -1) == 1))
			break;

		/* Add waiting tasks to the end of the waitqueue (FIFO) */
		waiter.tcb = current();
		list_add_tail(&waiter.list, &lock->waitqueue);

		/* didn't get the lock, go to sleep. */
		spin_unlock(&lock->wait_lock);

		waiting();

		BUG_ON(local_irq_is_enabled());

		spin_lock(&lock->wait_lock);
	}

	/* set it to 0 if there are no waiters left */
	if (likely(list_empty(&lock->waitqueue)))
		atomic_set(&lock->count, 0);

	skip_wait:

	/* got the lock - cleanup and rejoice! */
	lock->owner = current();

	spin_unlock_irqrestore(&lock->wait_lock, flags);

}

void mutex_unlock(struct mutex *lock) {

	struct mutex_waiter *waiter = NULL;
	bool need_resched = false;
	uint32_t flags;

	BUG_ON(!mutex_is_locked(lock));

	spin_lock_irqsave(&lock->wait_lock, flags);

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

	spin_lock_irqsave(&lock->wait_lock, flags);

	if (!list_empty(&lock->waitqueue)) {

		/* Get the waiting the first entry of this associated waitqueue */
		waiter = list_entry(lock->waitqueue.next, struct mutex_waiter, list);

		need_resched = true;

		list_del(&waiter->list);

		wake_up(waiter->tcb);

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

	atomic_set(&lock->count, 1);
	spin_lock_init(&lock->wait_lock);
	INIT_LIST_HEAD(&lock->waitqueue);

}




