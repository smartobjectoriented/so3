/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2021	   Nicolas Müller <nicolas.muller1@heig-vd.ch>
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

#include <compiler.h>
#include <thread.h>

#define DEFINE(sym, val) \
        asm volatile("\n->" #sym " %0 " #val : : "i" (val))

#define BLANK() asm volatile("\n->" : : )

int main(void)
{
	BLANK();

	DEFINE(OFFSET_TCB_CPU_REGS, 	offsetof(tcb_t, cpu_regs));

	BLANK();

	DEFINE(OFFSET_RA,			offsetof(cpu_regs_t, ra));
	DEFINE(OFFSET_SP,			offsetof(cpu_regs_t, sp));
	DEFINE(OFFSET_GP,			offsetof(cpu_regs_t, gp));
	DEFINE(OFFSET_TP,			offsetof(cpu_regs_t, tp));
	DEFINE(OFFSET_T0,			offsetof(cpu_regs_t, t0));
	DEFINE(OFFSET_T1,			offsetof(cpu_regs_t, t1));
	DEFINE(OFFSET_T2,			offsetof(cpu_regs_t, t2));
	DEFINE(OFFSET_FP,			offsetof(cpu_regs_t, fp));
	DEFINE(OFFSET_S1,			offsetof(cpu_regs_t, s1));
	DEFINE(OFFSET_A0,			offsetof(cpu_regs_t, a0));
	DEFINE(OFFSET_A1,			offsetof(cpu_regs_t, a1));
	DEFINE(OFFSET_A2,			offsetof(cpu_regs_t, a2));
	DEFINE(OFFSET_A3,			offsetof(cpu_regs_t, a3));
	DEFINE(OFFSET_A4,			offsetof(cpu_regs_t, a4));
	DEFINE(OFFSET_A5,			offsetof(cpu_regs_t, a5));
	DEFINE(OFFSET_A6,			offsetof(cpu_regs_t, a6));
	DEFINE(OFFSET_A7,			offsetof(cpu_regs_t, a7));
	DEFINE(OFFSET_S2,			offsetof(cpu_regs_t, s2));
	DEFINE(OFFSET_S3,			offsetof(cpu_regs_t, s3));
	DEFINE(OFFSET_S4,			offsetof(cpu_regs_t, s4));
	DEFINE(OFFSET_S5,			offsetof(cpu_regs_t, s5));
	DEFINE(OFFSET_S6,			offsetof(cpu_regs_t, s6));
	DEFINE(OFFSET_S7,			offsetof(cpu_regs_t, s7));
	DEFINE(OFFSET_S8,			offsetof(cpu_regs_t, s8));
	DEFINE(OFFSET_S9,			offsetof(cpu_regs_t, s9));
	DEFINE(OFFSET_S10,			offsetof(cpu_regs_t, s10));
	DEFINE(OFFSET_S11,			offsetof(cpu_regs_t, s11));
	DEFINE(OFFSET_T3,			offsetof(cpu_regs_t, t3));
	DEFINE(OFFSET_T4,			offsetof(cpu_regs_t, t4));
	DEFINE(OFFSET_T5,			offsetof(cpu_regs_t, t5));
	DEFINE(OFFSET_T6,			offsetof(cpu_regs_t, t6));
	DEFINE(OFFSET_STATUS,		offsetof(cpu_regs_t, status));
	DEFINE(OFFSET_EPC,			offsetof(cpu_regs_t, epc));

	return 0;
}
