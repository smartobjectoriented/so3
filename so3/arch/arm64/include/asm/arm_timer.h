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

#include <asm/processor.h>

/**
 * In AVZ and ARM64VT we are using the ARM physical timer. The guest domains will
 * rely on virtual timer where an offset can be added.
 */

/*
 * Ensure that reads of the counter are treated the same as memory reads
 * for the purposes of ordering by subsequent memory barriers.
 *
 * This insanity brought to you by speculative system register reads,
 * out-of-order memory accesses, sequence locks and Thomas Gleixner.
 *
 * http://lists.infradead.org/pipermail/linux-arm-kernel/2019-February/631195.html
 */
#define arch_counter_enforce_ordering(val) do {				\
	u64 tmp, _val = (val);						\
									\
	asm volatile(							\
	"	eor	%0, %1, %1\n"					\
	"	add	%0, sp, %0\n"					\
	"	ldr	xzr, [%0]"					\
	: "=r" (tmp) : "r" (_val));					\
} while (0)

/*
 * These register accessors are marked inline so the compiler can
 * nicely work out which register we want, and chuck away the rest of
 * the code. At least it does so with a recent GCC (4.6.3).
 */
#ifdef CONFIG_ARM64VT

static inline void arch_timer_reg_write_el2(enum arch_timer_reg reg, u32 val)
{
	switch (reg) {
	case ARCH_TIMER_REG_CTRL:
		write_sysreg(val, cnthp_ctl_el2);
		break;
	case ARCH_TIMER_REG_TVAL:
		write_sysreg(val, cnthp_tval_el2);
		break;
	}

	isb();
}

static inline u32 arch_timer_reg_read_el2(enum arch_timer_reg reg)
{
	switch (reg) {
	case ARCH_TIMER_REG_CTRL:
		return read_sysreg(cnthp_ctl_el2);
	case ARCH_TIMER_REG_TVAL:
		return read_sysreg(cnthp_tval_el2);
	}

	BUG();

	return 0;
}

static inline void arch_timer_reg_write_el0(enum arch_timer_reg reg, u32 val)
{
	switch (reg) {
	case ARCH_TIMER_REG_CTRL:
		write_sysreg(val, cntp_ctl_el0);
		break;
	case ARCH_TIMER_REG_TVAL:
		write_sysreg(val, cntp_tval_el0);
		break;
	}

	isb();
}

static inline u32 arch_timer_reg_read_el0(enum arch_timer_reg reg)
{
	switch (reg) {
	case ARCH_TIMER_REG_CTRL:
		return read_sysreg(cntp_ctl_el0);
	case ARCH_TIMER_REG_TVAL:
		return read_sysreg(cntp_tval_el0);
	}

	BUG();

	return 0;
}

#else

static inline void arch_timer_reg_write_cp15(int access, enum arch_timer_reg reg, u32 val)
{
	if (access == ARCH_TIMER_PHYS_ACCESS) {
		switch (reg) {
		case ARCH_TIMER_REG_CTRL:
			write_sysreg(val, cntp_ctl_el0);
			break;
		case ARCH_TIMER_REG_TVAL:
			write_sysreg(val, cntp_tval_el0);
			break;
		}
	} else if (access == ARCH_TIMER_VIRT_ACCESS) {
		switch (reg) {
		case ARCH_TIMER_REG_CTRL:
			write_sysreg(val, cntv_ctl_el0);
			break;
		case ARCH_TIMER_REG_TVAL:
			write_sysreg(val, cntv_tval_el0);
			break;
		}
	}

	isb();
}

static inline u32 arch_timer_reg_read_cp15(int access, enum arch_timer_reg reg)
{
	if (access == ARCH_TIMER_PHYS_ACCESS) {
			switch (reg) {
			case ARCH_TIMER_REG_CTRL:
				return read_sysreg(cntp_ctl_el0);
			case ARCH_TIMER_REG_TVAL:
				return read_sysreg(cntp_tval_el0);
			}
		} else if (access == ARCH_TIMER_VIRT_ACCESS) {
			switch (reg) {
			case ARCH_TIMER_REG_CTRL:
				return read_sysreg(cntv_ctl_el0);
			case ARCH_TIMER_REG_TVAL:
				return read_sysreg(cntv_tval_el0);
			}
		}

	BUG();

	return 0;
}

#endif /* CONFIG_ARM64VT */

/**
 * Get the timer frequency
 *
 * @return counter frequency at all ELs
 */
static inline u32 arch_timer_get_cntfrq(void)
{
	return read_sysreg(cntfrq_el0);
}

/**
 * Get the current value of the counter (typically used by the clocksource)
 *
 * @return Current 64-bit timer value
 */
static inline u64 arch_counter_get_cntvct(void)
{
	u64 cnt;

	isb();
#ifdef CONFIG_ARM64VT
	cnt = read_sysreg(cntpct_el0);
#else
	cnt = read_sysreg(cntvct_el0);
#endif
	arch_counter_enforce_ordering(cnt);

	return cnt;
}

static inline u32 arch_timer_get_cntkctl(void)
{
	return read_sysreg(cntkctl_el1);
}

static inline void arch_timer_set_cntkctl(u32 cntkctl)
{
	write_sysreg(cntkctl, cntkctl_el1);
	isb();
}

#endif /* ASM_ARM_TIMER_H */
