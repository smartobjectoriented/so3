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

#include <compiler.h>
#include <thread.h>
#include <signal.h>

#include <asm/processor.h>
#include <asm/types.h>

#include <asm/smccc.h>

#ifdef CONFIG_AVZ

#include <avz/sched.h>

#include <avz/uapi/avz.h>

#endif /* CONFIG_AVZ */

/* Use marker if you need to separate the values later */

#define DEFINE(sym, val) \
        asm volatile("\n->" #sym " %0 " #val : : "i" (val))

#define BLANK() asm volatile("\n->" : : )

int main(void)
{
#ifdef CONFIG_AVZ

	DEFINE(OFFSET_AVZ_SHARED, offsetof(struct domain, avz_shared));

	DEFINE(OFFSET_EVTCHN_UPCALL_PENDING, offsetof(struct avz_shared, evtchn_upcall_pending));

	DEFINE(OFFSET_HYPERVISOR_CALLBACK,  offsetof(struct avz_shared, vectors_vaddr));
	DEFINE(OFFSET_DOMCALL_CALLBACK, offsetof(struct avz_shared, domcall_vaddr));
	DEFINE(OFFSET_TRAPS_CALLBACK, offsetof(struct avz_shared, traps_vaddr));

	DEFINE(OFFSET_G_SP,		 offsetof(struct domain, g_sp));

	DEFINE(OFFSET_CPU_REGS,		offsetof(struct domain, cpu_regs));
#endif

	BLANK();

	DEFINE(OFFSET_X0,		offsetof(struct cpu_regs, x0));
	DEFINE(OFFSET_X1,		offsetof(struct cpu_regs, x1));
	DEFINE(OFFSET_X2,		offsetof(struct cpu_regs, x2));
	DEFINE(OFFSET_X3,		offsetof(struct cpu_regs, x3));
	DEFINE(OFFSET_X4,		offsetof(struct cpu_regs, x4));
	DEFINE(OFFSET_X5,		offsetof(struct cpu_regs, x5));
	DEFINE(OFFSET_X6,		offsetof(struct cpu_regs, x6));
	DEFINE(OFFSET_X7,		offsetof(struct cpu_regs, x7));
	DEFINE(OFFSET_X8,		offsetof(struct cpu_regs, x8));
	DEFINE(OFFSET_X9,		offsetof(struct cpu_regs, x9));
	DEFINE(OFFSET_X10,		offsetof(struct cpu_regs, x10));
	DEFINE(OFFSET_X11,		offsetof(struct cpu_regs, x11));
	DEFINE(OFFSET_X12,		offsetof(struct cpu_regs, x12));
	DEFINE(OFFSET_X13,		offsetof(struct cpu_regs, x13));
	DEFINE(OFFSET_X14,		offsetof(struct cpu_regs, x14));
	DEFINE(OFFSET_X15,		offsetof(struct cpu_regs, x15));
	DEFINE(OFFSET_X16,		offsetof(struct cpu_regs, x16));
	DEFINE(OFFSET_X17,		offsetof(struct cpu_regs, x17));
	DEFINE(OFFSET_X18,		offsetof(struct cpu_regs, x18));
	DEFINE(OFFSET_X19,		offsetof(struct cpu_regs, x19));
	DEFINE(OFFSET_X20,		offsetof(struct cpu_regs, x20));
	DEFINE(OFFSET_X21,		offsetof(struct cpu_regs, x21));
	DEFINE(OFFSET_X22,		offsetof(struct cpu_regs, x22));
	DEFINE(OFFSET_X23,		offsetof(struct cpu_regs, x23));
	DEFINE(OFFSET_X24,		offsetof(struct cpu_regs, x24));
	DEFINE(OFFSET_X25,		offsetof(struct cpu_regs, x25));
	DEFINE(OFFSET_X26,		offsetof(struct cpu_regs, x26));
	DEFINE(OFFSET_X27,		offsetof(struct cpu_regs, x27));
	DEFINE(OFFSET_X28,		offsetof(struct cpu_regs, x28));
	DEFINE(OFFSET_FP,		offsetof(struct cpu_regs, fp));
	DEFINE(OFFSET_LR,		offsetof(struct cpu_regs, lr));
	DEFINE(OFFSET_SP,		offsetof(struct cpu_regs, sp));
	DEFINE(OFFSET_PC,		offsetof(struct cpu_regs, pc));
	DEFINE(OFFSET_PSTATE,		offsetof(struct cpu_regs, pstate));

	BLANK();

	DEFINE(ARM_SMCCC_RES_X0_OFFS,		offsetof(struct arm_smccc_res, a0));
	DEFINE(ARM_SMCCC_RES_X2_OFFS,		offsetof(struct arm_smccc_res, a2));
	DEFINE(ARM_SMCCC_QUIRK_ID_OFFS,	offsetof(struct arm_smccc_quirk, id));
	DEFINE(ARM_SMCCC_QUIRK_STATE_OFFS,	offsetof(struct arm_smccc_quirk, state));

	BLANK();

	DEFINE(OFFSET_TCB_CPU_REGS, 	offsetof(tcb_t, cpu_regs));

	return 0;
}

