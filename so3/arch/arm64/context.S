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

.globl __get_syscall_args_ext
.globl __get_syscall_arg
.globl __get_syscall_stack_arg

.global __mmu_switch
.global __exec
.global __write
.global __save_context
.global __root_proc

.extern thread_prologue
.extern current_thread

.global __enable_vfp

.extern __check_ptrace_traceme
.extern ret_from_fork

#ifdef CONFIG_AVZ

// Switch from a domain to another
// Requires to re-configure the vbar_el1 vector table address
ENTRY(__switch_domain_to)
	mov		x10, #(OFFSET_CPU_REGS + OFFSET_X19)
	b	____switch_to
#endif /* CONFIG_AVZ */

ENTRY(__switch_to)
	mov		x10, #(OFFSET_TCB_CPU_REGS + OFFSET_X19)

____switch_to:
	cbz 	x0, load_ctx
	add		x8, x0, x10

save_ctx:

	stp		x19, x20, [x8], #16		// store callee-saved registers
	stp		x21, x22, [x8], #16
	stp		x23, x24, [x8], #16
	stp		x25, x26, [x8], #16
	stp		x27, x28, [x8], #16
	stp		x29, lr, [x8], #16

	mov		x9, sp
	str		x9, [x8]

load_ctx:

	// Prepare to retrieve the regs from the stack
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
// x0 contains the TTBR0 related to this CPU for the L0 page table

ENTRY(__mmu_switch_ttbr0_el1)
 	// Ensure the flushes happen before continuing
	dsb   sy

	// Ensure synchronization with previous code changes
	isb

	msr    ttbr0_el1, x0
	isb

	ret

// Switch the MMU to a L0 page table
// x0 contains the TTBR related to this CPU for the L0 page table

ENTRY(__mmu_switch_ttbr0)
 	// Ensure the flushes happen before continuing
	dsb   sy

	// Ensure synchronization with previous code changes
    isb
#ifdef CONFIG_AVZ

#ifdef CONFIG_ARM64VT
	msr ttbr0_el2, x0
#else
	msr	ttbr1_el1, x0
#endif /* !CONFIG_ARM64VT */

#else /* !CONFIG_AVZ */
	msr   ttbr0_el1, x0
#endif

    isb

	ret

// Switch the MMU to a L0 page table
// x0 contains the TTBR related to this CPU for the L0 page table

ENTRY(__mmu_switch_ttbr1)
 	// Ensure the flushes happen before continuing
	dsb   sy

	// Ensure synchronization with previous code changes
	isb

	msr    ttbr1_el1, x0
	isb

	ret

ENTRY(__mmu_switch_vttbr)
 	// Ensure the flushes happen before continuing
	dsb   	sy

	// Ensure synchronization with previous code changes
    isb

	/* Switch the vttbr to map the correct guest */
	msr   	vttbr_el2, x0
    isb

	tlbi 	alle2
	dsb		nsh

	ret

ENTRY(cpu_do_idle)

	dsb	  sy			// WFI may enter a low-power mode
	wfi

	ret

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
// x9 contains the address of the errno variable

__get_syscall_args_ext:

	str		x8, [x0]
	str		x9, [x1]

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

	// Adjust the kernel stack pointer so that we can proceed with ret_from_fork.
	// S_FRAME_SIZE/8 registers are preserved when at the syscall vector entry point

	// Adjust the sp which is stored on the stack. Make sure
	// it refers to this stack and not the one issue from the copy
	// as during fork().

	sub		x1, x1, #S_FRAME_SIZE

	// Prepare to configure sp during the context switch.
	str		x1, [x0, #(OFFSET_TCB_CPU_REGS + OFFSET_SP)]

	// Prepare the lr to branch to ret_from_fork
	ldr		x1, .LCret_from_fork
	str		x1, [x0, #(OFFSET_TCB_CPU_REGS + OFFSET_LR)]

	// Preserve x8 which contains the syscall number (used to compare against SIG_RETURN)
	str		x8, [x0, #(OFFSET_TCB_CPU_REGS + OFFSET_X8)]

	// The other registers are not important.

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
	mov x9, xzr

	// Invoke the syscall - kernel side
	svc	0

	// exec("sh.elf")

	adr	x0, .LC_exec

	// No args
	mov x1, xzr
	mov x2, xzr

	// No errno
	mov x9, xzr

	mov	x8, #SYSCALL_EXECVE

	svc 0

	// If we are here, exec() has failed...

	mov	x0, #STDOUT
	adr x1, .LCnoshell
	mov x2, noshell_len
	mov x8, #SYSCALL_WRITE

	// No errno
	mov x9, xzr

	svc 0

	b kernel_panic
