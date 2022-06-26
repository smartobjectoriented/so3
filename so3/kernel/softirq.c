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

#include <bitops.h>
#include <types.h>
#include <softirq.h>

#include <device/irq.h>

unsigned long softirq_bitmap;

static softirq_handler softirq_handlers[NR_SOFTIRQS];

/*
 * Perform actions of related pending softirqs if any.
 */
void do_softirq(void)
{
	unsigned int i;
	unsigned long pending;
	unsigned int loopmax;

	loopmax = 0;

	for ( ; ; )
	{
		/* Any pending softirq ? */
		if ((pending = softirq_bitmap) == 0)
			break;

		i = find_first_bit(&pending, BITS_PER_INT);

		if (loopmax > 100)   /* Probably something wrong ;-) */
			printk("%s: Warning trying to process softirq for quite a long time (i = %d)...\n", __func__, i);

		clear_bit(i, (unsigned long *) &softirq_bitmap);

		(*softirq_handlers[i])();

		/* If we left the interrupt context along a context switch... */
		__in_interrupt = true;

		loopmax++;
	}

	/* schedule() could have been invoked outside a softirq context, therefore we disable the interrupt context here. */
	__in_interrupt = false;
}

void register_softirq(int nr, softirq_handler handler)
{
    ASSERT(nr < NR_SOFTIRQS);

    softirq_handlers[nr] = handler;
}


void raise_softirq(unsigned int nr)
{
	set_bit(nr, (unsigned long *) &softirq_bitmap);
}


void softirq_init(void)
{
	softirq_bitmap = 0;
}
