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
#include <generated/asm-offsets.h>
#include <linkage.h>

#include <asm/processor.h>

.global ret_from_fork
.global x86_idt_stubs
.global x86_syscall_entry

/*
 * Every entry builds a cpu_regs_t on the kernel stack. The CPU pushes
 * ss/rsp/rflags/cs/rip, the stub adds the error code and the vector, and
 * save_regs the general registers plus the two TCB-only slots.
 */

.macro save_regs
	push	%rdi
	push	%rsi
	push	%rdx
	push	%rcx
	push	%rax
	push	%r8
	push	%r9
	push	%r10
	push	%r11
	push	%rbx
	push	%rbp
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	sub	$16, %rsp
	cld
.endm

.macro restore_regs
	add	$16, %rsp
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%rbp
	pop	%rbx
	pop	%r11
	pop	%r10
	pop	%r9
	pop	%r8
	pop	%rax
	pop	%rcx
	pop	%rdx
	pop	%rsi
	pop	%rdi

	/* Vector and error code */

	add	$16, %rsp
.endm

/* Exceptions 8, 10-14, 17, 21, 29 and 30 push an error code */

.macro idt_stub vec
	.align 16
	.if (\vec == 8) || ((\vec >= 10) && (\vec <= 14)) || (\vec == 17) || (\vec == 21) || (\vec == 29) || (\vec == 30)
	.else
	push	$0
	.endif
	push	$\vec
	jmp	x86_common_entry
.endm

.text

.align 16
x86_idt_stubs:
	.altmacro
	.set	vec, 0
	.rept	48
	idt_stub %vec
	.set	vec, vec + 1
	.endr

x86_common_entry:
	save_regs

	mov	%rsp, %rdi
	call	trap_handle

	testb	$3, OFFSET_CS(%rsp)
	jnz	ret_to_user

	restore_regs
	iretq

/*
 * syscall: rcx = user rip, r11 = user rflags, IF cleared by SFMASK.
 * The stack is still the user one; the frame is built as if the CPU had
 * taken an interrupt, so the exit path is shared.
 */

x86_syscall_entry:
	mov	%rsp, x86_user_rsp(%rip)
	mov	x86_kernel_rsp(%rip), %rsp

	push	$USER_DS
	push	x86_user_rsp(%rip)
	push	%r11
	push	$USER_CS
	push	%rcx
	push	$0
	push	$X86_SYSCALL_VECTOR

	save_regs

	mov	%rsp, %rdi
	call	syscall_entry_handle

#ifdef CONFIG_IPC_SIGNAL
	/*
	 * rt_sigreturn: drop the frame of the signal handler, so that the
	 * interrupted one below is restored.
	 */

	test	%rax, %rax
	jz	ret_to_user

	add	$S_FRAME_SIZE, %rsp
#endif

/* Entry point for new user threads as well, with rsp on their frame */

ret_from_fork:
ret_to_user:
#ifdef CONFIG_IPC_SIGNAL
	call	sig_check
	test	%rax, %rax
	jz	1f

	/* Build the frame of the user handler below the current one */

	mov	%rsp, %rsi
	sub	$S_FRAME_SIZE, %rsp
	mov	%rsp, %rdi
	mov	%rax, %rdx
	call	x86_setup_sig_frame
1:
#endif
	/* The next entry from user mode lands right above this frame */

	lea	S_FRAME_SIZE(%rsp), %rdi
	call	x86_set_kernel_stack

	restore_regs
	iretq

.data
.align 8

.global x86_kernel_rsp
x86_kernel_rsp:
	.quad	0

x86_user_rsp:
	.quad	0

.section .note.GNU-stack, "", @progbits
