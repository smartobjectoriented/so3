/*
 * Copyright (C) 2026 Dieperink Clément <clement.dieperink@heig-vd.ch>
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

#include <syscall.h>
#include <thread.h>
#include <process.h>

/* ARM32 specific syscalls to manage TLS */
#define ARM_NR_set_tls 0x0f0005
#define ARM_NR_get_tls 0x0f0006

extern void __get_syscall_args_ext(long *syscall_no);

/**
 * Handle special ARM32 syscalls, or fallback to generic syscall handling.
 */
long arch_syscall_handle(syscall_args_t *syscall_args)
{
	long syscall_no;
	cpu_regs_t *user_regs = (cpu_regs_t *) arch_get_kernel_stack_frame(current());

	/* Get addtional args of the syscall according to the ARM & SO3 ABI */
	__get_syscall_args_ext(&syscall_no);

	switch (syscall_no) {
	case ARM_NR_get_tls:
		return user_regs->tls_usr;
	case ARM_NR_set_tls:
		user_regs->tls_usr = syscall_args->args[0];
		return 0;
	default:
		return syscall_handle(syscall_args);
	}
}
