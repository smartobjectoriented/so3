/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 *
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

#include <calibrate.h>
#include <schedule.h>
#include <timer.h>

#include <device/timer.h>

/**
 * Set the global variable jiffy_usec with the number of loop for 1us
 */
void calibrate_delay(void) {
	u64 __jiffies = jiffies;

	printk("%s: calibrating...", __func__);

	jiffies_ref = 0;

	if (!periodic_timer.start && !oneshot_timer.start) {
		lprintk("Warning! No timer initialized yet.\n");
		return ;
	}

#ifdef CONFIG_RTOS
	oneshot_timer.set_delay(NSECS / HZ);
#endif /* CONFIG_RTOS */

	while (jiffies == __jiffies) ;
	__jiffies = jiffies;

#ifdef CONFIG_RTOS
	oneshot_timer.set_delay(NSECS / HZ);
#endif /* CONFIG_RTOS */

	while (jiffies == __jiffies) jiffies_ref++;

	printk("done. jiffies_ref = %llx\n", jiffies_ref);

}
