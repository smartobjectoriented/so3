/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#define DEFINE(sym, val) asm volatile("\n->" #sym " %0 " #val : : "i"(val))

#define BLANK() asm volatile("\n->" : :)

int main(void)
{
	DEFINE(OFFSET_KSP, offsetof(struct cpu_regs, ksp));
	DEFINE(OFFSET_FS_BASE, offsetof(struct cpu_regs, fs_base));
	DEFINE(OFFSET_RAX, offsetof(struct cpu_regs, rax));
	DEFINE(OFFSET_RDI, offsetof(struct cpu_regs, rdi));
	DEFINE(OFFSET_RSI, offsetof(struct cpu_regs, rsi));
	DEFINE(OFFSET_VECTOR, offsetof(struct cpu_regs, vector));
	DEFINE(OFFSET_RIP, offsetof(struct cpu_regs, rip));
	DEFINE(OFFSET_CS, offsetof(struct cpu_regs, cs));
	DEFINE(OFFSET_RFLAGS, offsetof(struct cpu_regs, rflags));
	DEFINE(OFFSET_RSP, offsetof(struct cpu_regs, rsp));
	DEFINE(OFFSET_SS, offsetof(struct cpu_regs, ss));
	DEFINE(S_FRAME_SIZE, sizeof(struct cpu_regs));

	BLANK();

	DEFINE(OFFSET_TCB_CPU_REGS, offsetof(tcb_t, cpu_regs));

	BLANK();

	DEFINE(OFFSET_SYS_SIGNUM, offsetof(__sigaction_t, signum));
	DEFINE(OFFSET_SYS_SA, offsetof(__sigaction_t, sa));

	BLANK();

	DEFINE(OFFSET_SA_HANDLER, offsetof(sigaction_t, sa_handler));
	DEFINE(OFFSET_SA_RESTORER, offsetof(sigaction_t, sa_restorer));

	return 0;
}
