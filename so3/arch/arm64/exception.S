/*
 * Copyright (C) 2021-2024 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <common.h>

#include <generated/autoconf.h>

#include <asm-offsets.h>
#include <syscall.h>

#include <asm/processor.h>

#ifdef CONFIG_AVZ

#include <avz/domain.h>

.global cpu_entrypoint

.globl	hypervisor_stack
.global upcall_path

#endif /* CONFIG_AVZ */

.globl ret_from_fork
.global __call_sig_handler
.extern __sync_serror
.extern do_exit
.extern __check_ptrace_syscall
.extern sig_check

.global __vectors

.extern __start

/*
 * Four types of exceptions:
 * - synchronous: aborts from MMU, SP/CP alignment checking, unallocated
 *   instructions, SVCs/SMCs/HVCs, ...)
 * - IRQ: group 1 (normal) interrupts
 * - FIQ: group 0 or secure interrupts
 * - SError: fatal system errors
 *
 * Four different contexts:
 * - from same exception level, when using the SP_EL0 stack pointer
 * - from same exception level, when using the SP_ELx stack pointer
 * - from lower exception level, when this is AArch64
 * - from lower exception level, when this is AArch32
 *
 * +------------------+------------------+-------------------------+
 * |     Address      |  Exception type  |       Description       |
 * +------------------+------------------+-------------------------+
 * | VBAR_ELn + 0x000 | Synchronous      | Current EL with SP0     |
 * |          + 0x080 | IRQ / vIRQ       |                         |
 * |          + 0x100 | FIQ / vFIQ       |                         |
 * |          + 0x180 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x200 | Synchronous      | Current EL with SPx     |
 * |          + 0x280 | IRQ / vIRQ       |                         |
 * |          + 0x300 | FIQ / vFIQ       |                         |
 * |          + 0x380 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x400 | Synchronous      | Lower EL using AArch64  |
 * |          + 0x480 | IRQ / vIRQ       |                         |
 * |          + 0x500 | FIQ / vFIQ       |                         |
 * |          + 0x580 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x600 | Synchronous      | Lower EL using AArch32  |
 * |          + 0x680 | IRQ / vIRQ       |                         |
 * |          + 0x700 | FIQ / vFIQ       |                         |
 * |          + 0x780 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 */

/* use the special section (.vectors.text), to enable fine-tuning
 * of the placement of this section inside the linker script
 */
.section ".vectors.text", "ax"

	b __start  // To be compliant with reset vector (unavailable in aarch64)

.align 12
ENTRY(__vectors)

	// Current EL with SP0 / Synchronous
	.align 7

	mov		x0, lr
	b 		trap_handle_error

	// Current EL with SP0 / IRQ
	.align 7

	mov		x0, lr
	b 		trap_handle_error

	// Current EL with SP0 / FIQ
	.align 7

	mov		x0, lr
	b 		trap_handle_error

	// Current EL with SP0 / SError
	.align 7

	mov		x0, lr
	b 		trap_handle_error

	// Current EL with SPx / Synchronous
	.align 7
	
	mov		x0, lr
	b 		trap_handle_error

	// Current EL with SPx / IRQ
	.align 7

#ifdef CONFIG_AVZ
	b 		el1_2_12_irq_handler
#else /* CONFIG_AVZ */
	b 		el1_irq_handler
#endif /* !CONFIG_AVZ */

	// Current EL with SPx / FIQ
	.align 7

	mov		x0, lr
	b 		trap_handle_error

	// Current EL with SPx / SError
	.align 7

	mov		x0, lr
	mrs		x1, esr_el1

	b		__sync_serror

	// Lower EL using AArch64 / Synchronous
	.align 7

	// This vector is concerned with the syscall interrupt.

#ifdef CONFIG_AVZ
	b 		el12_sync_handler
#else  
	b 		el01_sync_handler
#endif  

	// Lower EL using AArch64 / IRQ
	.align 7

#ifdef CONFIG_AVZ
	b 		el1_2_12_irq_handler
#else /* CONFIG_AVZ */
	b 		el0_irq_handler
#endif /* !CONFIG_AVZ */

	// Lower EL using AArch64 / FIQ
	.align 7

	mov		x0, lr
	b 		trap_handle_error

	// Lower EL using AArch64 / SError
	.align 7

	mov		x0, lr
	b 		trap_handle_error

	// Lower EL using AArch32 / Synchronous
	.align 7

	mov		x0, lr
	b 		trap_handle_error

	// Lower EL using AArch32 / IRQ
	.align 7

	mov		x0, lr
	b 		trap_handle_error

	// Lower EL using AArch32 / FIQ
	.align 7

	mov		x0, lr
	b 		trap_handle_error

	// Lower EL using AArch32 / SError
	.align 7

	mov		x0, lr
	b 		trap_handle_error

__prepare_sig_handler:
#if 0
	str		sp, [sp, #(OFFSET_SP-S_FRAME_SIZE)]	// save sp

	// Build a new stack frame based on the current
	sub		sp, sp, #S_STACK_FRAME_SIZE

	// Make sure the spsr is with Thumb de-activated to perform normal execution of the handler

	mov		r1, #PSR_USR_MODE		// Ensure the handler will run in user mode (situation where
								// the current frame inherits from code running in SVC).

	str		r1, [sp, #OFFSET_PSR]		// Save the updated SPSR

	// Set the argument (signum, handler) to r0 & r1
	ldr		r1, [r0, #OFFSET_SYS_SIGNUM]
	str		r1, [sp, #OFFSET_R0]

	ldr 	r0, [r0, #OFFSET_SYS_SA]
	ldr		r1, [r0, #OFFSET_SA_HANDLER]
	str		r1, [sp, #OFFSET_R1]

	ldr		r1, [r0, #OFFSET_SA_RESTORER]
	str		r1, [sp, #OFFSET_PC]		// Set the handler to the PC

	/* Set the current sp_usr to have a valid stack in the user space */

	ldr 	r0, .LCcurrent
	ldr		r0, [r0]
	ldr 	r0, [r0, #(OFFSET_TCB_CPU_REGS + OFFSET_SP_USR)]
	str		r0, [sp, #OFFSET_SP_USR]

	mov		pc, lr				// Back to the caller
#endif
	ret

#ifdef CONFIG_CPU_SPIN_TABLE

ENTRY(pre_ret_to_el1_with_spin)

	mov x1, x0
	str xzr, [x1]
1:
	wfe
 	ldr x0, [x1]

    	cbz x0, 1b

    	// Branch to the given address
	msr		elr_el2, x0

	// Set the CPU in EL1 mode to proceed with
	// the bootstrap of the domain

	mov		x2, #PSR_MODE_EL1t

	// Make sure no interrupt coming from CPU #0 is
	// interferring with other CPU bootstrap
	orr		x2, x2, #PSR_I_BIT

	msr		spsr_el2, x2

	// According to boot protocol
	mov		x1, #0
	mov		x1, #0
	mov		x2, #0
	mov		x3, #0

	// Ready to jump into the Linux domain...

	eret

	/*
	 * Mitigate Straight-line Speculation.
	 * Guard against Speculating past an ERET instruction and
	 * potentially perform speculative accesses to memory before
	 * processing the exception return
	 */
	dsb nsh
	isb

	nop
	nop
	nop
#endif /* CONFIG_CPU_SPIN_TABLE */

#ifdef CONFIG_AVZ

#ifdef CONFIG_CPU_PSCI
ENTRY(pre_ret_to_el1)

	wfi

	ldr 		x0, cpu_entrypoint
	msr		elr_el2, x0

	// Set the CPU in EL1 mode to proceed with
	// the bootstrap of the domain

	mov		x2, #PSR_MODE_EL1t

	// Make sure no interrupt coming from CPU #0 is
	// interferring with other CPU bootstrap
	orr		x2, x2, #PSR_I_BIT

	msr		spsr_el2, x2

	// According to boot protocol
	mov		x1, #0
	mov		x1, #0
	mov		x2, #0
	mov		x3, #0

	// Ready to jump into the Linux domain...

	eret

	/*
	 * Mitigate Straight-line Speculation.
	 * Guard against Speculating past an ERET instruction and
	 * potentially perform speculative accesses to memory before
	 * processing the exception return
	 */
	dsb nsh
	isb

	nop
	nop
	nop

#endif /* CONFIG_CPU_PSCI */

.align 5
el12_sync_handler:

	kernel_entry

	mrs		x0, spsr_el2
	str		x0, [sp, #OFFSET_PSTATE]

	mrs		x0, sp_el1
	str		x0, [sp, #OFFSET_SP]

	mrs		x0, elr_el2
	str		x0, [sp, #OFFSET_PC]

 	mov 		x0, sp
	bl		trap_handle

	//str		x0, [sp, #OFFSET_X0]

	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el2, x0

	ldr		x0, [sp, #OFFSET_SP]
	msr		sp_el1, x0

	ldr		x0, [sp, #OFFSET_PC]
	msr		elr_el2, x0

	kernel_exit

	eret

.align  5
el1_2_12_irq_handler:

	kernel_entry

	mrs		x0, elr_el2
	str		x0, [sp, #OFFSET_PC]

	mrs		x0, spsr_el2
	str		x0, [sp, #OFFSET_PSTATE]

	mrs		x0, sp_el1
	str		x0, [sp, #OFFSET_SP]

	// Make sure r0 refers to the base of the stack frame
	mov		x0, sp

	// The stack must stay 16-byte aligned

	str		lr, [sp, #-16]!
    	bl 		irq_handle
	ldr		lr, [sp], #16

	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el2, x0

	ldr		x0, [sp, #OFFSET_PC]
	msr		elr_el2, x0

	ldr		x0, [sp, #OFFSET_SP]
	msr		sp_el1, x0

  	kernel_exit

	eret

cpu_entrypoint:
	.quad 0x0

/*
 * This function is called at bootstrap and
 * reboot time. It initializes some registers
 */
ENTRY(pre_ret_to_user)

	// Initial state - IRQs off
	disable_irq

	// Get a reference to our domain descriptor
	curdom	x10, x11

	ldr	x2, [sp, #OFFSET_PC]  // Entry point of the guest
	msr	elr_el2, x2

	/* 
	 * The MMU must be disabled so that the guest can keep its initial 
	 * ake sure CPU EL1 has MMU disabled
	*/

	mrs	x2, sctlr_el1
	bic	x2, x2, #SCTLR_ELx_M
	msr	sctlr_el1, x2

	// Set the CPU in EL1 mode to proceed with
	// the bootstrap of the domain

	mov	x2, #(PSR_MODE_EL1h | PSR_F_BIT	| PSR_I_BIT | PSR_A_BIT | PSR_D_BIT)

	msr	spsr_el2, x2

	ldr	x0, [sp, #OFFSET_X21] // Device tree (fdt_addr)

	// According to boot protocol
	mov	x1, #0
	mov	x2, #0
	mov	x3, #0

	// Ready to jump into the domain...

	eret

	/*
	 * Mitigate Straight-line Speculation.
	 * Guard against Speculating past an ERET instruction and
	 * potentially perform speculative accesses to memory before
	 * processing the exception return
	 */
	dsb nsh
	isb

__crash:
	b __crash

hypervisor_stack:
	.space CONFIG_NR_CPUS * 8

.LChypervisor_stack:
	.quad	hypervisor_stack

#endif /* CONFIG_AVZ */

.macro check_pending_signal
	// Is there any pending signals for this process?
	bl		sig_check

	cmp		x0, #0
	b.eq	1f

	// Configure a stack frame to manage the user handler
	bl		__prepare_sig_handler
1:

.endm

el01_sync_handler:

	// If the syscall interrupt occurs with the SO3-on-avz configuration,
	// the stack frame is built in AVZ (interrupt vector at this level).
	// Hence, the SO3 domain (guest) will just need to preserve the
	// return address and make the local processing.

	kernel_entry

	mrs		x0, elr_el1
	str		x0, [sp, #OFFSET_PC]

	mrs		x0, sp_el0
	str		x0, [sp, #OFFSET_SP]

	mrs		x0, spsr_el1
	str		x0, [sp, #OFFSET_PSTATE]

	// Make sure r0 refers to the base of the stack frame
	mov		x0, sp

   	bl 		trap_handle

__ret_from_fork:

#ifdef CONFIG_IPC_SIGNAL

	// Is there any pending signals for this process?
	check_pending_signal

#endif /* CONFIG_IPC_SIGNAL */


	ldr		x0, [sp, #OFFSET_PC]
	msr		elr_el1, x0

	ldr 		x0, [sp, #OFFSET_SP]
	msr		sp_el0, x0

	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el1, x0

	kernel_exit

	eret

.align  5
el0_irq_handler:

	kernel_entry

	mrs		x0, elr_el1
	str		x0, [sp, #OFFSET_PC]

	mrs		x0, sp_el0
	str		x0, [sp, #OFFSET_SP]

	mrs		x0, spsr_el1
	str		x0, [sp, #OFFSET_PSTATE]

	// Make sure r0 refers to the base of the stack frame
	mov		x0, sp

   	bl 		irq_handle

	ldr		x0, [sp, #OFFSET_PC]
	msr		elr_el1, x0

	ldr 		x0, [sp, #OFFSET_SP]
	msr		sp_el0, x0

	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el1, x0

	kernel_exit

	eret


.align  5
el1_irq_handler:

	kernel_entry

	mrs		x0, elr_el1
	str		x0, [sp, #OFFSET_PC]

	mrs		x0, spsr_el1
	str		x0, [sp, #OFFSET_PSTATE]

	// Make sure r0 refers to the base of the stack frame
	mov		x0, sp

	// The stack must stay 16-byte aligned

   	bl 		irq_handle

	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el1, x0

	ldr		x0, [sp, #OFFSET_PC]
	msr		elr_el1, x0

  	kernel_exit

	eret


// Used at entry point of a fork'd process (setting the return value to 0)
ret_from_fork:

	str		xzr, [sp, #OFFSET_X0]

	b		__ret_from_fork

// IRQs are off
// ARM EABI: the syscall nr is stored in x8
.align  5
syscall_interrupt:

#if 0 /* Still to be ported in ARM64 */

	// At the exception entry, the stack must be 8-byte aligned.
	// If it is not the case (gcc might not respect the AAPCS convention for optimization purposes),
	// sp will be adjusted. The original sp is preserved and will be correctly restored at the exit.

	tst		sp, #0x7	// 8-bytes aligned
	strne	sp, [sp, #(OFFSET_SP-SVC_STACK_FRAME_SIZE - 4)]	// save sp
	subne	sp, sp, #4
	streq	sp, [sp, #(OFFSET_SP-SVC_STACK_FRAME_SIZE)]	// save sp

	// Alignment guard
	tst		sp, #0x7		// 8-bytes aligned
	bne		__stack_alignment_fault

	// Build the stack frame to store registers

	sub		sp, sp, #SVC_STACK_FRAME_SIZE

	str		lr, [sp, #OFFSET_LR]	// save lr in lr
	str		lr, [sp, #OFFSET_PC]	// save lr in pc

	stmia 	sp, {r0-r12}  	// Store registers

    mrs 	lr, spsr        		// Get spsr
	str		lr, [sp, #OFFSET_PSR]	// Store spsr

	// Saving user mode registers (sp_usr, lr_usr)
	add		lr, sp, #OFFSET_SP_USR
	stmia	lr, {sp, lr}^

	ldr		r0, [sp, #OFFSET_SP_USR]
	ldr 	r1, .LCcurrent
	ldr 	r1, [r1]

	str 	r0, [r1, #(OFFSET_TCB_CPU_REGS + OFFSET_SP_USR)]

	// Restore r0-r2
	ldmia	sp, {r0-r2}

#ifdef CONFIG_MMU
	// Give a chance to a ptrace tracer to monitor us (before the syscall)
	stmfd 	sp!, {r0-r4}
	bl		__check_ptrace_syscall
	ldmfd 	sp!, {r0-r4}
#endif

	cpsie   i 			// Re-enable interrupts
    bl 		syscall_handle
    cpsid	i			// Re-disable interrupts to be safe in regs manipulation

	// Check if sigreturn has been called. In this case, we
	// clean the stack frame which has been used to manage the user handler.
	cmp		r7, #SYSCALL_SIGRETURN
	bne		__no_sigreturn

	// Reset the stack frame by removing the one issued from sigreturn
	add		sp, sp, #SVC_STACK_FRAME_SIZE
#endif

__no_sigreturn:

#if 0
#ifdef CONFIG_MMU
	// Give a chance to a ptrace tracer to monitor us (after the syscall)
	stmfd 	sp!, {r0-r4}
	bl	__check_ptrace_syscall
	ldmfd 	sp!, {r0-r4}
#endif
#endif
 
#if !defined(CONFIG_AVZ) && defined(CONFIG_SOO)

.align 5

ENTRY(__avz_hypercall)

	hvc #0
	ret

// We may reach this point only during along the upcall
// path of a hypercall return.

.align 5
ENTRY(avz_vector_callback)

	// x0 is set to cpu_regs_t by avz
	bl 		irq_handle

	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el1, x0

	ldr		x0, [sp, #OFFSET_SP]
	msr		sp_el0, x0

	ldr		x0, [sp, #OFFSET_PC]
	msr		elr_el1, x0

	kernel_exit

	eret

#endif


