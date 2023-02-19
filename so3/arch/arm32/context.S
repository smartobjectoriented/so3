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

#include <generated/autoconf.h>

#include <asm/assembler.h>
#include <asm-offsets.h>
#include <asm/processor.h>
#include <asm/mmu.h>

#include <syscall.h>
#include <vfs.h>
#include <linkage.h>

.global __switch_context
.global __thread_prologue_kernel
.global __thread_prologue_user
.global __exec_prologue_user
.global __thread_prologue_user_pre_launch

.globl __get_syscall_args_ext
.globl __get_syscall_arg
.globl __get_syscall_stack_arg

.global __mmu_switch_ttbr0
.global __exec
.global __write
.global __save_context

.global __enable_vfp

.extern thread_prologue
.extern current_thread

#ifdef CONFIG_MMU

.extern __check_ptrace_traceme
.extern ret_from_fork
.extern pre_launch_proc

#endif

@ Enable VFP
__enable_vfp:

    mov r1, #0x40000000
    vmsr fpexc, r1 ; fpexc = r1
    bx lr

@ Get the arguments of the syscall which are stored on the stack
@ (greather than 4 args (r0-r3))
@ r0 contains the number of args (0 = first argument on the stack,
@ hence the fifth argument of the syscall, 1 = second arg. on the stack,
@ hence the sixth arg., etc.
@ Returns the value in r0 (according to the EABI)
__get_syscall_stack_arg:

	ldr	r1, .LCcurrent
	ldr r1, [r1]

	@ Get the user stack pointer at the entry of the interrupt vector
	ldr	r1, [r1, #(OFFSET_TCB_CPU_REGS + OFFSET_SP_USR)]

	@ The syscall stub uses r7, r10 and r11. These are NO scratch register
	@ and are preserved on the stack before the exception.
	@ Therefore, we have to skip 12 bytes.
	add r1, r1, #12

	ldr r0, [r1, r0, lsl #2]

	mov pc, lr

@ Get the additional arguments linked to the syscall.
@ The ABI convention is described in crt0.S of the libc.
@ r7 contains the syscall number
@ r10 contains the address of the errno variable
__get_syscall_args_ext:

	str		r7, [r0]
	str		r10, [r1]

	mov 	pc, lr

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
	bic	r4, r4, #PSR_MODE_MASK
	orr	r4, r4, #PSR_USR_MODE
 	msr	 cpsr, r4

	@ User stack initialisation
	mov  sp, r6

	bl	thread_prologue


#ifdef CONFIG_AVZ

ENTRY(cpu_do_idle)

	dsb	  sy			// WFI may enter a low-power mode
	wfi

	mov pc,	lr

/*
 * r0 = previous domain, r1 = next domain
 * previous and next are guaranteed not to be the same.
 *
 */
ENTRY(__switch_domain_to)
	disable_irq 				@ ensure IRQs are disabled

	add     ip, r0, #(OFFSET_CPU_REGS + OFFSET_R4)
@ save_ctx
    stmia   ip, {r4 - sl, fp, ip, sp, lr}      @ Store most regs on stack

	current_cpu r6

@ load_ctx

	add		ip, r1, #(OFFSET_CPU_REGS + OFFSET_R4)

    ldmia   ip,  {r4 - sl, fp, ip, sp, pc}       @ Load all regs saved previously

	nop
	nop
	nop

#endif /* CONFIG_AVZ */

@ r0: address of tcb prev
@ r1: address of tcb next
@ IRQs are disabled and SVC mode
ENTRY(__switch_to)

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
@ r1 and r2 are used as scratch registers.

__mmu_switch_ttbr0:

	dsb
	isb

	@ So far, we do not use ASID/Context ID

	mrc p15, 0, r1, c2, c0, 0
	isb

	ldr	r2, =TTBR0_BASE_ADDR_MASK
	bic r1, r1, r2

	orr r1, r1, r0

	mcr	p15, 0, r1, c2, c0, 0		@ set TTBR0

	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0		@ I+BTB cache invalidate

	dsb
	isb

	mov		pc, lr

	nop
	nop
	nop

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

	@ Preserve r7 which contains the syscall number (used to compare against SIG_RETURN)
	str		r7, [r0, #(OFFSET_TCB_CPU_REGS + OFFSET_R7)]

	@ The other registers are not important.

	mov 	pc, lr

.LCret_from_fork:
	.word ret_from_fork

.LCcurrent:
	.word current_thread

@ This section contains all data and code which will be
@ mapped in the user space as initial process to be executed.

.section ".root_proc.text","ax"

	b	__root_proc

.LC_welcome:
	.string "SO3: starting the initial process (shell) ...\n\n\n"
	.equ welcome_len, . - .LC_welcome

.LC_exec:
	.string "sh.elf"

.LCnoshell:
	.string "so3: No init proc(shell) found ...\n"
	.equ noshell_len, . - .LCnoshell


.align 2

@ This is the initial code which runs in user mode
@ to start the first process image

ENTRY(__root_proc)

	@ write(stdout, buf, len)

	mov	r0, #STDOUT
	adr r1, .LC_welcome
	mov r2, #welcome_len
	mov r7, #SYSCALL_WRITE

	@ No errno
	mov r10, #0

	@ Invoke the syscall - kernel side
	svc	0

	@ exec("sh.elf")

	adr	r0, .LC_exec
	@ No args
	mov r1, #0
	mov r2, #0
	@ No errno
	mov r10, #0

	mov	r7, #SYSCALL_EXECVE

	svc 0
