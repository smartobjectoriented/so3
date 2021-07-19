/*
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
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

#include <asm/processor.h>

#define SBI_EXT_TIME 			0x54494D45
#define SBI_EXT_TIME_SET_TIMER  0

struct sbiret {
	long error;
	long value;
};

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
			unsigned long arg1, unsigned long arg2,
			unsigned long arg3, unsigned long arg4,
			unsigned long arg5)
{
	struct sbiret ret;

	register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);
	register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);
	register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);
	register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);
	register uintptr_t a4 asm ("a4") = (uintptr_t)(arg4);
	register uintptr_t a5 asm ("a5") = (uintptr_t)(arg5);
	register uintptr_t a6 asm ("a6") = (uintptr_t)(fid);
	register uintptr_t a7 asm ("a7") = (uintptr_t)(ext);
	asm volatile ("ecall"
		      : "+r" (a0), "+r" (a1)
		      : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
		      : "memory");
	ret.error = a0;
	ret.value = a1;

	return ret;
}

static inline u32 arch_timer_get_cntfrq(void)
{
	/* It seems RISC-V time registers reflect a real time wall-clock to avoid multiple frequency on multiple
	 * hardware */
	return NSECS;
}

static void sbi_set_timer(uint64_t stime_value)
{
	sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, stime_value, 0,
		  0, 0, 0, 0);
}

static inline u64 arch_get_time(void) {

		u64 n;

		__asm__ __volatile__ (
			"rdtime %0"
			: "=r" (n));

		return n;
}

#endif /* ARM_TIMER_H */

