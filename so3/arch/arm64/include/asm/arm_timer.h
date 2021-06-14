/*
 * Copyright (C) 2014-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef ASM_ARM_TIMER_H
#define ASM_ARM_TIMER_H

#include <device/arch/arm_timer.h>

/*
 * These register accessors are marked inline so the compiler can
 * nicely work out which register we want, and chuck away the rest of
 * the code. At least it does so with a recent GCC (4.6.3).
 */
static inline void arch_timer_reg_write(enum arch_timer_reg reg, u32 val)
{
	switch (reg) {
	case ARCH_TIMER_REG_CTRL:
		//asm volatile("mcr p15, 0, %0, c14, c3, 1" : : "r" (val));
		break;

	case ARCH_TIMER_REG_TVAL:
		//asm volatile("mcr p15, 0, %0, c14, c3, 0" : : "r" (val));
		break;
	}

	isb();
}

static inline u32 arch_timer_reg_read(enum arch_timer_reg reg)
{
	u32 val = 0;

	switch (reg) {
	case ARCH_TIMER_REG_CTRL:
		//asm volatile("mrc p15, 0, %0, c14, c3, 1" : "=r" (val));
		break;

	case ARCH_TIMER_REG_TVAL:
		//asm volatile("mrc p15, 0, %0, c14, c3, 0" : "=r" (val));
		break;
	}


	return val;
}

static inline u32 arch_timer_get_cntfrq(void)
{
	u32 val = 0;

	//asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r" (val));

	return val;
}

static inline u64 arch_counter_get_cntvct(void)
{
	u64 cval = 0;

	isb();
	//asm volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (cval));

	return cval;
}

static inline u32 arch_timer_get_cntkctl(void)
{
	u32 cntkctl;

	//asm volatile("mrc p15, 0, %0, c14, c1, 0" : "=r" (cntkctl));

	return cntkctl;
}

static inline void arch_timer_set_cntkctl(u32 cntkctl)
{
	//asm volatile("mcr p15, 0, %0, c14, c1, 0" : : "r" (cntkctl));
}

#endif /* ASM_ARM_TIMER_H */
