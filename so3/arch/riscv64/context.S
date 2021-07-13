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

# Manage various context-related code (context switch)

#if 0 /* _NMR_ */
#include <asm/assembler.h>
#include <asm-offsets.h>
#include <asm/processor.h>
#include <asm/mmu.h>
#endif /* _NMR_ */

#include <linkage.h>

#define SYSCALL_EXEC_NR		2
#define SYSCALL_WRITE_NR	5

.global __switch_context

.global __thread_prologue_kernel
.global __thread_prologue_user
.global __exec_prologue_user
.global __thread_prologue_user_pre_launch

.globl __get_syscall_args_ext
.globl __get_syscall_arg
.globl __get_syscall_stack_arg

.global __mmu_switch
.global __exec
.global __write
.global __save_context

.global __enable_vfp

#if 0 /* _NMR_ */
.extern thread_prologue
.extern current_thread

#ifdef CONFIG_MMU

.extern __check_ptrace_traceme
.extern ret_from_fork
.extern pre_launch_proc

#endif

#endif /* _NMR_ */

__enable_vfp:

__get_syscall_stack_arg:

__get_syscall_args_ext:

__exec:

__write:

__thread_prologue_kernel:

__thread_prologue_user:

__switch_context:

save_ctx:

load_ctx:

__mmu_switch:

__save_context:

#if 0 /* _NMR_ */
.LCret_from_fork:
	.word ret_from_fork

.LCcurrent:
	.word current_thread
#endif /* _NMR_ */
