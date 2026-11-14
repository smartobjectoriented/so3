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

#include <generated/autoconf.h>
#include <generated/syscall_number.h>
#include <linkage.h>

#include <vfs.h>

#include <asm/processor.h>

.global __x86_switch_stack
.global __thread_prologue_kernel
.global __root_proc

.text

/*
 * __x86_switch_stack(u64 *prev_ksp, u64 next_ksp)
 *
 * The callee-saved registers and the FXSAVE image of the outgoing thread stay
 * on its kernel stack; the word right after the image holds the stack pointer
 * to return to. arch_prepare_cpu_regs() lays out the same for a new thread.
 */

ENTRY(__x86_switch_stack)
	push	%rbp
	push	%rbx
	push	%r12
	push	%r13
	push	%r14
	push	%r15

	mov	%rsp, %rax
	sub	$(X86_FXSAVE_SIZE + 16), %rsp
	and	$-16, %rsp
	fxsave64 (%rsp)
	mov	%rax, X86_FXSAVE_SIZE(%rsp)
	mov	%rsp, (%rdi)

	mov	%rsi, %rsp
	fxrstor64 (%rsp)
	mov	X86_FXSAVE_SIZE(%rsp), %rsp

	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%rbx
	pop	%rbp

	ret

/* First run of a kernel thread: rbx = th_fn, r12 = th_arg */

ENTRY(__thread_prologue_kernel)
	mov	%rbx, %rdi
	mov	%r12, %rsi

	sti

	call	thread_prologue

	/* Never reached */

1:
	hlt
	jmp	1b

ENTRY(cpu_do_idle)
	sti
	hlt
	ret

/*
 * Data and code mapped in the user space as the initial process. The code
 * runs at USER_SPACE_VADDR, hence only rip-relative references.
 */

.section ".root_proc.text","ax"

	jmp	__root_proc

.LC_welcome:
	.string "[SO3 USR] Starting the initial process\n"
	.equ welcome_len, . - .LC_welcome

.LC_exec:
	.string CONFIG_USER_INIT_PROGRAM

.LCnoshell:
	.string "[SO3 USR] No init process found\n"
	.equ noshell_len, . - .LCnoshell

.align 16

ENTRY(__root_proc)

	/* write(stdout, buf, len) */

	mov	$STDOUT, %edi
	lea	.LC_welcome(%rip), %rsi
	mov	$welcome_len, %edx
	mov	$SYSCALL_write, %eax
	syscall

	/* execve(init, NULL, NULL) */

	lea	.LC_exec(%rip), %rdi
	xor	%esi, %esi
	xor	%edx, %edx
	mov	$SYSCALL_execve, %eax
	syscall

	/* If we are here, exec() has failed... */

	mov	$STDOUT, %edi
	lea	.LCnoshell(%rip), %rsi
	mov	$noshell_len, %edx
	mov	$SYSCALL_write, %eax
	syscall

1:
	jmp	1b

.section .note.GNU-stack, "", @progbits
