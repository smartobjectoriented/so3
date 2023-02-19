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

	DEFINE(OFFSET_TCB_CPU_REGS, 	offsetof(tcb_t, cpu_regs));

	BLANK();

	DEFINE(OFFSET_R0,			offsetof(cpu_regs_t, r0));
	DEFINE(OFFSET_R1,			offsetof(cpu_regs_t, r1));
	DEFINE(OFFSET_R2,			offsetof(cpu_regs_t, r2));
	DEFINE(OFFSET_R3,			offsetof(cpu_regs_t, r3));
	DEFINE(OFFSET_R4,			offsetof(cpu_regs_t, r4));
	DEFINE(OFFSET_R5,			offsetof(cpu_regs_t, r5));
	DEFINE(OFFSET_R6,			offsetof(cpu_regs_t, r6));
	DEFINE(OFFSET_R7,			offsetof(cpu_regs_t, r7));
	DEFINE(OFFSET_R8,			offsetof(cpu_regs_t, r8));
	DEFINE(OFFSET_R9,			offsetof(cpu_regs_t, r9));
	DEFINE(OFFSET_R10,			offsetof(cpu_regs_t, r10));
	DEFINE(OFFSET_FP,			offsetof(cpu_regs_t, fp));
	DEFINE(OFFSET_IP,			offsetof(cpu_regs_t, ip));
	DEFINE(OFFSET_SP,			offsetof(cpu_regs_t, sp));
	DEFINE(OFFSET_LR,			offsetof(cpu_regs_t, lr));
	DEFINE(OFFSET_PC,			offsetof(cpu_regs_t, pc));
	DEFINE(OFFSET_PSR,			offsetof(cpu_regs_t, psr));
	DEFINE(OFFSET_SP_USR,			offsetof(cpu_regs_t, sp_usr));
	DEFINE(OFFSET_LR_USR,			offsetof(cpu_regs_t, lr_usr));

	BLANK();

	DEFINE(OFFSET_SYS_SIGNUM,		offsetof(__sigaction_t, signum));
	DEFINE(OFFSET_SYS_SA,			offsetof(__sigaction_t, sa));

	BLANK();

	DEFINE(OFFSET_SA_HANDLER,		offsetof(sigaction_t, sa_handler));
	DEFINE(OFFSET_SA_RESTORER,		offsetof(sigaction_t, sa_restorer));

	return 0;
}

