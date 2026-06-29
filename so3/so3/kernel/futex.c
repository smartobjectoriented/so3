/*
 * Copyright (C) 2025 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
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

#include <process.h>
#include <heap.h>
#include <errno.h>
#include <futex.h>

/**
 * do_futex_wait - block on futex_w
 *
 * @param futex_w address of the futex word
 * @param val expected value of the futex word
 * @return 0 on success or error value
 */
static int do_futex_wait(uint32_t *futex_w, uint32_t val, const struct timespec *utime)
{
	unsigned long flags;
	pcb_t *pcb = current()->pcb;
	struct list_head *pos;
	futex_t *futex;
	queue_thread_t f_element;

	if (utime)
		printk("[futex] utime parameter is not used in current implementation\n");

	flags = spin_lock_irqsave(&pcb->futex_lock);

	if (*futex_w != val) {
		spin_unlock_irqrestore(&pcb->futex_lock, flags);
		return -EAGAIN;
	}

	/* look if a futex_w already exists */
	list_for_each(pos, &pcb->futex) {
		futex = list_entry(pos, futex_t, list);

		if ((uintptr_t) futex_w == futex->key)
			break;
	}

	if (list_is_head(pos, &pcb->futex)) {
		/* no futex on futex_w */
		futex = (futex_t *) calloc(1, sizeof(futex_t));
		if (futex == NULL)
			BUG();

		futex->key = (uintptr_t) futex_w;

		INIT_LIST_HEAD(&futex->f_element);
		list_add_tail(&futex->list, &pcb->futex);
	}

	f_element.tcb = current();

	list_add_tail(&f_element.list, &futex->f_element);

	/* go to sleep. */
	spin_unlock(&pcb->futex_lock);
	waiting();

	BUG_ON(local_irq_is_enabled());

	spin_lock(&pcb->futex_lock);

	if (list_empty(&futex->f_element)) {
		list_del(&futex->list);
		free(futex);
	}

	spin_unlock_irqrestore(&pcb->futex_lock, flags);

	return 0;
}

/**
 * do_futex_wake - wake one or more tasks blocked on uaddr
 *
 * @nr_wake wake up to this many tasks
 * @return the number of waiters that were woken up
 */
static int do_futex_wake(uint32_t *futex_w, uint32_t nr_wake)
{
	unsigned long flags;
	pcb_t *pcb = current()->pcb;
	struct list_head *pos, *p;
	futex_t *futex;
	queue_thread_t *f_element;
	unsigned idx = 0;

	flags = spin_lock_irqsave(&pcb->futex_lock);

	/* Search for the futex element with futex_w as key */
	list_for_each(pos, &pcb->futex) {
		futex = list_entry(pos, futex_t, list);

		if ((uintptr_t) futex_w == futex->key)
			break;
	}

	/* Check if the wanted key was found in the list */
	if (list_is_head(pos, &pcb->futex)) {
		/* key does not exists in futex - Error */
		spin_unlock_irqrestore(&pcb->futex_lock, flags);
		return -EINVAL;
	}

	/* wakes at most nr_wake of the waiters that are waiting */
	list_for_each_safe(pos, p, &futex->f_element) {
		f_element = list_entry(pos, queue_thread_t, list);

		if (idx == nr_wake)
			break;

		list_del(&f_element->list);
		ready(f_element->tcb);

		idx++;
	}

	spin_unlock_irqrestore(&pcb->futex_lock, flags);

	return idx;
}

SYSCALL_DEFINE6(futex, uint32_t *, uaddr, int, op, uint32_t, val, const struct timespec *, utime, uint32_t *, uaddr2, uint32_t,
		val3)
{
	int cmd = op & FUTEX_CMD_MASK;

	switch (cmd) {
	case FUTEX_WAIT:
		return do_futex_wait(uaddr, val, utime);
	case FUTEX_WAKE:
		return do_futex_wake(uaddr, val);
	case FUTEX_FD:
	case FUTEX_REQUEUE:
	case FUTEX_CMP_REQUEUE:
	case FUTEX_WAKE_OP:
	case FUTEX_LOCK_PI:
	case FUTEX_UNLOCK_PI:
	case FUTEX_TRYLOCK_PI:
	case FUTEX_WAIT_BITSET:
		printk("Futex cmd '%d' not supported !\n", cmd);
		return -EINVAL;
	}

	return -ENOSYS;
}
