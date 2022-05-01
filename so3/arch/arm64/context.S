/*
 * Copyright (C) 2022 Daniel Rossier <daniel.rossier//heig-vd.ch>
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

// Manage various context-related code (context switch)

#include <generated/asm-offsets.h>

#include <syscall.h>
#include <vfs.h>

#include <asm/processor.h>
#include <asm/mmu.h>

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
.global __root_proc

.global __enable_vfp

.extern thread_prologue
.extern current_thread

#ifdef CONFIG_MMU

.extern __check_ptrace_traceme
.extern ret_from_fork
.extern pre_launch_proc

#endif

// Get the arguments of the syscall which are stored on the stack
// (greather than 4 args (r0-r3))
// r0 contains the number of args (0 = first argument on the stack,
// hence the fifth argument of the syscall, 1 = second arg. on the stack,
// hence the sixth arg., etc.
// Returns the value in r0 (according to the EABI)
__get_syscall_stack_arg:

#if 0
	ldr	r1, .LCcurrent
	ldr r1, [r1]

	// Get the user stack pointer at the entry of the interrupt vector
	ldr	r1, [r1, #(OFFSET_TCB_CPU_REGS + OFFSET_SP_USR)]

	// The syscall stub uses r7, r10 and r11. These are NO scratch register
	// and are preserved on the stack before the exception.
	// Therefore, we have to skip 12 bytes.
	add r1, r1, #12

	ldr r0, [r1, r0, lsl #2]

	mov pc, lr
#endif
	ret

// Get the additional arguments linked to the syscall.
// The ABI convention is described in crt0.S of the libc.
// x8 contains the syscall number
// x10 contains the address of the errno variable

__get_syscall_args_ext:

	str		x8, [x0]
	str		x10, [x1]

	ret

// exec syscall used by the kernel to run the initial binary image (shell)
__exec:
#if 0
  	stmfd sp!, {r7, r10, r11, r12}
  	mov	r7, #SYSCALL_EXEC_NR

  	mov r1, #0 		// args = NULL
  	mov r2, #0 		// envp = NULL

	mov r10, #0  	// We do not use errno

  	swi 0

  	ldmfd sp!, {r7, r10, r11, r12}

  	mov	pc, lr
#endif
	ret

// Kernel thread initial entry point
// Called once per thread

__thread_prologue_kernel:

	// Prepare to jump into C code
	mov x0, x19		// tcb->th_fn
	mov x1, x20		// tcb->th_arg

	enable_irq

	bl	thread_prologue
	// Never reached

// User thread initial entry point
// Called once per thread
// x19: th_fn, x20: th_arg, x21: user stack

__thread_prologue_user:

  	msr elr_el1, x19

	msr sp_el0, x21

	mov	x0, #PSR_MODE_EL0t

	msr spsr_el1, x0

	// Prepare to jump into C code
	mov x0, x20

#if 0
	at s1e0r, x19
 	isb
 	mrs x0, par_el1
#endif

	eret
#if 0
	// Prepare to jump into C code
	mov r0, r4 // tcb->th_fn
	mov r1, r5 // tcb->th_arg


#ifdef CONFIG_MMU
	// Check if the thread must stopped because of ptrace/tracee
	stmfd sp!, {r0, r1}
	bl	__check_ptrace_traceme
	ldmfd sp!, {r0, r1}
#endif

	// IRQ enabling - must be done in SVC mode of course ;-)
	// We should take care about protecting against signal receipt:
	// since the stack is not initialized yet, the signal processing should be kept disabled.
	cpsie   i

 	// Switch into user mode
 	mrs  r4, cpsr
	bic	r4, r4, #PSR_MODE_MASK
	orr	r4, r4, #PSR_USR_MODE
 	msr	 cpsr, r4

	// User stack initialisation
	mov  sp, r6

	bl	thread_prologue
#endif

// Store the current registers into a cpu_regs structure passed in r0 (as first argument)
__save_context:

#if 0
	// Adjust the kernel stack pointer so that we can proceed with ret_from_fork
	// SVC_STACK_FRAME_SIZE/4 registers are preserved when at the syscall vector entry point

	// Adjust the sp which is stored on the stack. Make sure
	// it refers to this stack and not the one issue from the copy
	// as during fork().

	str		r1, [r1, #(OFFSET_SP-SVC_STACK_FRAME_SIZE)]

	sub		r2, r1, #SVC_STACK_FRAME_SIZE

	// Prepare to configure sp during the context switch.
	str		r2, [r0, #(OFFSET_TCB_CPU_REGS + OFFSET_SP)]

	// Prepare the lr to branch to ret_from_fork
	ldr		r1, .LCret_from_fork
	str		r1, [r0, #(OFFSET_TCB_CPU_REGS + OFFSET_LR)]

	// The other registers are not important.

	mov 	pc, lr
#endif
	ret

ENTRY(__switch_context)

	mov		x10, #(OFFSET_TCB_CPU_REGS + OFFSET_X19)

	cbz 	x0, load_ctx

	add		x8, x0, x10
	mov		x9, sp

save_ctx:

	stp		x19, x20, [x8], #16		// store callee-saved registers
	stp		x21, x22, [x8], #16
	stp		x23, x24, [x8], #16
	stp		x25, x26, [x8], #16
	stp		x27, x28, [x8], #16
	stp		x29, lr, [x8], #16

	str		x9, [x8]

	// Prepare to retrieve the regs from the stack
	add		x8, x1, x10

load_ctx:

	add		x8, x1, x10

	ldp		x19, x20, [x8], #16		// restore callee-saved registers
	ldp		x21, x22, [x8], #16
	ldp		x23, x24, [x8], #16
	ldp		x25, x26, [x8], #16
	ldp		x27, x28, [x8], #16
	ldp		x29, lr, [x8], #16

	ldr		x9, [x8]
	mov		sp, x9

	ret

// Switch the MMU to a L0 page table
// x0 contains the TTBR related to this CPU for the L0 page table

ENTRY(__mmu_switch)

	dsb   sy                     /* Ensure the flushes happen before continuing */
    isb                          /* Ensure synchronization with previous changes to text */

	// Only the user space range is concerned with a MMU context switch
	msr    ttbr1_el1, x0

    isb

	ret

ENTRY(cpu_do_idle)

	dsb	  sy			// WFI may enter a low-power mode
	wfi

	ret

.LCret_from_fork:
	.quad ret_from_fork

.LCcurrent:
	.quad current_thread

// This section contains all data and code which will be
// mapped in the user space as initial process to be executed.

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

// This is the initial code which runs in user mode
// to start the first process image

ENTRY(__root_proc)

	// write(stdout, buf, len)

	mov	x0, #STDOUT
	adr x1, .LC_welcome
	mov x2, welcome_len
	mov x8, #SYSCALL_WRITE

	// No errno
	mov x10, xzr

	// Invoke the syscall - kernel side
	svc	0

	// exec("sh.elf")

	adr	x0, .LC_exec
	// No args
	mov x1, xzr
	mov x2, xzr
	// No errno
	mov x10, xzr

	mov	x8, #SYSCALL_EXECVE

	svc 0


#if 0
	/* Start the first process */
	__exec("sh.elf");
	__arch_user_exec(...)

	/* We normally never runs here, if the exec() succeeds... */
	__arch_user_write(..)
	printk("so3: No init proc (shell) found ...\n");

	__arch_user_kernel_panic();

	kernel_panic();
#endif

