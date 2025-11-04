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
 * do_futex_wait -
 *
 * @param futex_w
 */
static int do_futex_wait(uint32_t *futex_w, uint32_t val)
{
	unsigned long flags;
	pcb_t *pcb = current()->pcb;
	spinlock_t f_lock = pcb->futex_lock;
	struct list_head *pos;
	futex_t *futex;
	futex_el_t *f_element;

	flags = spin_lock_irqsave(&f_lock);

	if (*futex_w != val)
		return -EAGAIN;

	/* look if a futex_w already exists */
	list_for_each(pos, &pcb->futex) {
		futex = list_entry(pos, futex_t, list);

		if ((uintptr_t)futex_w == futex->key)
			break;
	}

	if (pos == &pcb->futex) {
		/* no futex on futex_w */
		futex = (futex_t *)calloc(1, sizeof(futex_t));
		if (futex == NULL)
			BUG();

		list_add_tail(&futex->list, &pcb->futex);
	}

	/* Add the thread in the futex_element list */
	f_element = (futex_el_t *)calloc(1, sizeof(futex_el_t));
	if (f_element == NULL)
		BUG();

	f_element->tcb = current();

	list_add_tail(&f_element->list, &futex->f_element);

	/* go to sleep. */
	spin_unlock(&f_lock);
	waiting();

	BUG_ON(local_irq_is_enabled());

	spin_unlock_irqrestore(&f_lock, flags);

	return 0;
}

SYSCALL_DEFINE6(futex, uint32_t *, uaddr, int, op, uint32_t, val,
		const struct timespec *, utime,
		uint32_t *, uaddr2, uint32_t, val3)
{

	// unsigned int flags = futex_to_flags(op);
	int cmd = op & FUTEX_CMD_MASK;

	switch (cmd) {
	case FUTEX_WAIT:
		return do_futex_wait(uaddr, val);
	case FUTEX_WAKE:
		break;
	default:
		printk("Futex cmd '%d' not supported !\n");
		return -EINVAL;
	}

	return -ENOSYS;
}


void futex_init(void)
{

}



