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

@ Manage various context-related code (context switch)

#include <asm/assembler.h>
#include <asm-offsets.h>
#include <asm/processor.h>
#include <asm/mmu.h>

#include <linkage.h>

#define SYSCALL_EXEC_NR		2

.global __switch_context
.global __thread_prologue_kernel
.global __thread_prologue_user
.global __exec_prologue_user
.global __thread_prologue_user_pre_launch

.global __mmu_switch
.global __exec
.global __save_context

.extern thread_prologue

#ifdef CONFIG_MMU

.extern __check_ptrace_traceme
.extern ret_from_fork
.extern pre_launch_proc

#endif

@ exec syscall used by the kernel to run the initial binary image (shell)
__exec:
  	stmfd sp!, {r7, r10, r11, r12}
  	mov	r7, #SYSCALL_EXEC_NR

  	mov r1, #0 		@ args = NULL
  	mov r2, #0 		@ envp = NULL

	mov r10, #0  	@ We do not use errno

  	swi 0

  	ldmfd sp!, {r7, r10, r11, r12}

  	mov	pc, lr

@ Kernel thread initial entry point
@ Called once per thread
__thread_prologue_kernel:

	@ Prepare to jump into C code
	mov r0, r4 		@ tcb->th_fn
	mov r1, r5 		@ tcb->th_arg

	cpsie   i

	bl	thread_prologue

@ User thread initial entry point
@ Called once per thread
@ r4: th_fn, r5: th_arg, r6: user stack
__thread_prologue_user:

	@ Prepare to jump into C code
	mov r0, r4 @ tcb->th_fn
	mov r1, r5 @ tcb->th_arg

#ifdef CONFIG_MMU
	@ Check if the thread must stopped because of ptrace/tracee
	stmfd sp!, {r0, r1}
	bl	__check_ptrace_traceme
	ldmfd sp!, {r0, r1}
#endif

	@ IRQ enabling - must be done in SVC mode of course ;-)
	@ We should take care about protecting against signal receipt:
	@ since the stack is not initialized yet, the signal processing should be kept disabled.
	cpsie   i

 	@ Switch into user mode
 	mrs  r4, cpsr
	bic	r4, r4, #MODE_MASK
	orr	r4, r4, #USR_MODE
 	msr	 cpsr, r4

	@ User stack initialisation
	mov  sp, r6

	bl	thread_prologue


@ r0: address of tcb prev
@ r1: address of tcb next
@ IRQs are disabled and SVC mode
__switch_context:

	cmp	r0, #0
	beq	load_ctx

	add     ip, r0, #(OFFSET_TCB_CPU_REGS + OFFSET_R4)
save_ctx:

	stmia   ip, {r4 - r10, fp, ip, sp, lr}      @ Store most regs on stack

load_ctx:

	add		ip, r1, #(OFFSET_TCB_CPU_REGS + OFFSET_R4)

	ldmia   ip,  {r4 - r10, fp, ip, sp, pc}       @ Load all regs saved previously

	nop
	nop
	nop

@ Switch the MMU to a L1 page table
@ r0 contains the physical address of the L1 page table
__mmu_switch:
	mov	r2, #0
	orr	r0, r0, #TTB_FLAGS_SMP

	mcr	p15, 0, r2, c7, c5, 6		@ flush BTAC/BTB

	dsb

_out:
	mcr		p15, 0, r0, c2, c0, 0	@ set TTB 0
	mcr		p15, 0, r0, c2, c0, 1	@ set TTB 1

	isb

	mov		pc, lr

@ Store the current registers into a cpu_regs structure passed in r0 (as first argument)
__save_context:

	@ Adjust the kernel stack pointer so that we can proceed with ret_from_fork
	@ SVC_STACK_FRAME_SIZE/4 registers are preserved when at the syscall vector entry point

	@ Adjust the sp which is stored on the stack. Make sure
	@ it refers to this stack and not the one issue from the copy
	@ as during fork().

	str		r1, [r1, #(OFFSET_SP-SVC_STACK_FRAME_SIZE)]

	sub		r2, r1, #SVC_STACK_FRAME_SIZE

	@ Prepare to configure sp during the context switch.
	str		r2, [r0, #(OFFSET_TCB_CPU_REGS + OFFSET_SP)]

	@ Prepare the lr to branch to ret_from_fork
	ldr		r1, .LCret_from_fork
	str		r1, [r0, #(OFFSET_TCB_CPU_REGS + OFFSET_LR)]

	@ The other registers are not important.

	mov 	pc, lr

.LCret_from_fork:
	.word ret_from_fork
