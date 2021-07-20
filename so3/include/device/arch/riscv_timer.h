/*
 * Copyright (C) 2021 Nicolas Müller <nicolas.muller1@heig-vd.ch>
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

#ifndef ARM_TIMER_H
#define ARM_TIMER_H

#include <types.h>
#include <timer.h>

#include <asm/processor.h>


static inline u32 arch_timer_get_cntfrq(void)
{
	/* It seems RISC-V time registers reflect a real time wall-clock to avoid multiple frequency on multiple
	 * hardware */
	return NSECS;
}

static inline u64 arch_get_time(void) {

		u64 n;

		__asm__ __volatile__ (
			"rdtime %0"
			: "=r" (n));

		return n;
}

/*TODO  _NMR_ no irqs yet */
static void timer_isr(void) {
	printk("Hello from ISR\n");
}


#endif /* ARM_TIMER_H */

