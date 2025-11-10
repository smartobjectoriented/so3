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
.global __exec_prologue_user

.globl __get_syscall_args_ext
.globl __get_syscall_arg

.global __mmu_switch
.global __exec
.global __write
.global __root_proc

.extern thread_prologue
.extern current_thread

.global __enable_vfp

#ifdef CONFIG_AVZ

// Switch from a domain to another
ENTRY(__switch_domain_to)
	mov	x10, #(OFFSET_CPU_REGS + OFFSET_X19)
	b	____switch_to
#endif /* CONFIG_AVZ */

// We preserve the registers which are not save on the stack but
// are necessary to keep this running context.
ENTRY(__switch_to)
	mov	x10, #(OFFSET_TCB_CPU_REGS + OFFSET_X19)

____switch_to:
	cbz 	x0, load_ctx
	add	x8, x0, x10

save_ctx:
	// store callee-saved registers
	stp	x19, x20, [x8], #16
	stp	x21, x22, [x8], #16
	stp	x23, x24, [x8], #16
	stp	x25, x26, [x8], #16
	stp	x27, x28, [x8], #16
	stp	x29, lr, [x8], #16

	mov	x9, sp
	str	x9, [x8]

load_ctx:

	// Prepare to retrieve the regs from the stack
	add	x8, x1, x10

	// restore callee-saved registers
	ldp	x19, x20, [x8], #16
	ldp	x21, x22, [x8], #16
	ldp	x23, x24, [x8], #16
	ldp	x25, x26, [x8], #16
	ldp	x27, x28, [x8], #16
	ldp	x29, lr, [x8], #16

	ldr	x9, [x8]
	mov	sp, x9

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
	msr 	ttbr0_el2, x0
#else /* !CONFIG_AVZ */
	msr   	ttbr0_el1, x0
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

	msr	ttbr1_el1, x0

	tlbi	alle1

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

	tlbi 	vmalls12e1is
	tlbi	alle2

	dsb	nsh

	ret

ENTRY(cpu_do_idle)

	dsb	  sy			// WFI may enter a low-power mode
	wfi

	ret

// Get the additional arguments linked to the syscall.
// The ABI convention is described in crt0.S of the libc.
// x8 contains the syscall number

__get_syscall_args_ext:

	str	x8, [x0]

	ret

// Kernel thread initial entry point
// Called once per thread

__thread_prologue_kernel:

	// Prepare to jump into C code
	mov 	x0,	x19		// tcb->th_fn
	mov	x1,	x20		// tcb->th_arg

	enable_irq

	bl	thread_prologue
	// Never reached

.LCcurrent:
	.quad 	current_thread

// This section contains all data and code which will be
// mapped in the user space as initial process to be executed.

.section ".root_proc.text","ax"

	b	__root_proc

.LC_welcome:
	.string "[SO3 USR] Starting the initial process\n"
	.equ welcome_len, . - .LC_welcome

.LC_exec:
	.string CONFIG_USER_INIT_PROGRAM

.LCnoshell:
	.string "[SO3 USR] No init process found\n"
	.equ noshell_len, . - .LCnoshell


.align 2

// This is the initial code which runs in user mode
// to start the first process image

ENTRY(__root_proc)

	// write(stdout, buf, len)

	mov	x0, #STDOUT
	adr 	x1, .LC_welcome
	mov 	x2, welcome_len
	mov 	x8, #SYSCALL_write

	// Invoke the syscall - kernel side
	svc	0

	// exec("sh.elf")

	adr	x0, .LC_exec

	// No args
	mov 	x1, xzr
	mov 	x2, xzr

	mov	x8, #SYSCALL_execve

	svc 	0

	// If we are here, exec() has failed...

	mov	x0, #STDOUT
	adr 	x1, .LCnoshell
	mov 	x2, noshell_len
	mov 	x8, #SYSCALL_write

	svc 	0

	b 	kernel_panic
