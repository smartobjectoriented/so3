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

#include <common.h>
#include <thread.h>
#include <process.h>
#include <delay.h>
#include <spinlock.h>

#include <device/timer.h>

spinlock_t spinlock;
int threads = 0;
int count = 0;

void *thread_example(void *arg)
{
	unsigned long ii = 0;

	//printk("### entering thread_example.\n");

	for (ii = 0; ii < 10000000; ii++) {
		spin_lock(&spinlock);
		count++;
		count++;
		count++;
		count++;
		count++;
		count++;
		count++;
		count++;
		count++;
		count++;
		spin_unlock(&spinlock);
	}
	threads++;

	return NULL;
}

/*
 * Main entry point of so3 app in kernel standalone configuration.
 * Mainly for debugging purposes.
 */
void *app_thread_main(void *args)
{
	tcb_t *t1, *t2, *t3, *t4;

	/* Kernel never returns ! */
	printk("***********************************************\n");
	printk("Going to infinite loop...\n");
	printk("Kill Qemu with CTRL-a + x or reset the board\n");
	printk("***********************************************\n");

	spin_lock_init(&spinlock);

	t1 = kernel_thread(thread_example, "fn1", (void *) 1, 0);
	t2 = kernel_thread(thread_example, "fn2", (void *) 2, 0);
	t3 = kernel_thread(thread_example, "fn2", (void *) 3, 0);
	t4 = kernel_thread(thread_example, "fn2", (void *) 4, 0);

	while (threads != 4) ;


	printk("### Total = %lld\n", count);
	
	while(1);


	return NULL;
}
