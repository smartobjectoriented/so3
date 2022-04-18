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
#include <generated/asm-offsets.h>

#include <asm/processor.h>

.globl ret_from_fork
.global __call_sig_handler

.global __vectors

.extern current_thread
.extern irq_handle
.extern trap_handle

.extern dumpstack

.extern __sync_serror

.extern __prefetch_abort
.extern __data_abort
.extern __undefined_instruction

.extern do_exit
.extern schedule_isr
.extern __check_ptrace_syscall
.extern sig_check
.extern __stack_alignment_fault

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

.align 11
ENTRY(__vectors)

	// Current EL with SP0 / Synchronous
	.align 7
	b	not_used

	// Current EL with SP0 / IRQ
	.align 7
	b 	not_used

	// Current EL with SP0 / FIQ
	.align 7
	b 	not_used

	// Current EL with SP0 / SError
	.align 7
	b	not_used

	// Current EL with SPx / Synchronous
	.align 7

	mrs	x0, esr_el1

	b	not_used

	// Current EL with SPx / IRQ
	.align 7

	b 	el1_irq_handler

	// Current EL with SPx / FIQ
	.align 7
	b	not_used

	// Current EL with SPx / SError
	.align 7

	mov	x0, lr
	mrs	x1, esr_el1

	b	__sync_serror

	// Lower EL using AArch64 / Synchronous
	.align 7

	b	el0_sync_handler

	// Lower EL using AArch64 / IRQ
	.align 7

	b 	el0_irq_handler

	// Lower EL using AArch64 / FIQ
	.align 7
	b	not_used

	// Lower EL using AArch64 / SError
	.align 7
	b	not_used

	// Lower EL using AArch32 / Synchronous
	.align 7
	b	not_used

	// Lower EL using AArch32 / IRQ
	.align 7
	b	not_used

	// Lower EL using AArch32 / FIQ
	.align 7
	b	not_used

	// Lower EL using AArch32 / SError
	.align 7
	b	not_used



.align  5
not_used:
    b not_used

.macro kernel_entry
	sub		sp, sp, #S_FRAME_SIZE

	stp		x0, x1, [sp, #OFFSET_X0]
	stp		x2, x3, [sp, #OFFSET_X2]
	stp		x4, x5, [sp, #OFFSET_X4]
	stp		x6, x7, [sp, #OFFSET_X6]
	stp		x8, x9, [sp, #OFFSET_X8]
	stp		x10, x11, [sp, #OFFSET_X10]
	stp		x12, x13, [sp, #OFFSET_X12]
	stp		x14, x15, [sp, #OFFSET_X14]
	stp		x16, x17, [sp, #OFFSET_X16]
	stp		x18, x19, [sp, #OFFSET_X18]
	stp		x20, x21, [sp, #OFFSET_X20]
	stp		x22, x23, [sp, #OFFSET_X22]
	stp		x24, x25, [sp, #OFFSET_X24]
	stp		x26, x27, [sp, #OFFSET_X26]
	stp		x28, x29, [sp, #OFFSET_X28]
.endm

.macro kernel_exit

	ldp		x0, x1, [sp, #OFFSET_X0]
	ldp		x2, x3, [sp, #OFFSET_X2]
	ldp		x4, x5, [sp, #OFFSET_X4]
	ldp		x6, x7, [sp, #OFFSET_X6]
	ldp		x8, x9, [sp, #OFFSET_X8]
	ldp		x10, x11, [sp, #OFFSET_X10]
	ldp		x12, x13, [sp, #OFFSET_X12]
	ldp		x14, x15, [sp, #OFFSET_X14]
	ldp		x16, x17, [sp, #OFFSET_X16]
	ldp		x18, x19, [sp, #OFFSET_X18]
	ldp		x20, x21, [sp, #OFFSET_X20]
	ldp		x22, x23, [sp, #OFFSET_X22]
	ldp		x24, x25, [sp, #OFFSET_X24]
	ldp		x26, x27, [sp, #OFFSET_X26]
	ldp		x28, x29, [sp, #OFFSET_X28]

	add		sp, sp, #S_FRAME_SIZE
.endm


#if 0
// exception handlers
.align  5
undefined_instruction:

    b  __undefined_instruction

// Prepare to call a handler associated to a pending signal
// r0 contains the reference to the sigaction_t structure related to the signal to be processed.
#endif

__prepare_sig_handler:

#if 0
	// Alignment guard
	tst		sp, #0x7	// 8-bytes aligned
	bne		__stack_alignment_fault

	str		sp, [sp, #(OFFSET_SP-SVC_STACK_FRAME_SIZE)]	// save sp
	// Build a new stack frame based on the current
	sub		sp, sp, #SVC_STACK_FRAME_SIZE

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

#if 0
.macro check_pending_signal
	// Is there any pending signals for this process?
	bl		sig_check

	cmp		r0, #0
	beq		1f

	// Configure a stack frame to manage the user handler
	bl		__prepare_sig_handler
1:

.endm
#endif


// IRQs are off
// ARM EABI: the syscall nr is stored in r7
.align  5
syscall_interrupt:


#if 0

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

__ret_from_fork:

#if 0
	// Store the return value on the stack frame
	cmp		r7, #SYSCALL_SIGRETURN
	strne	r0, [sp, #OFFSET_R0]

#ifdef CONFIG_IPC_SIGNAL
	// Is there any pending signals for this process?

	check_pending_signal
#endif /* CONFIG_IPC_SIGNAL */

    ldr 	lr, [sp, #OFFSET_PSR]	// get the saved spsr and adjust the stack pointer
    msr		spsr, lr

	// Restoring user mode registers (sp_usr, lr_usr)
	add		lr, sp, #OFFSET_SP_USR
	ldmia	lr, {sp, lr}^

 	ldmia 	sp, {r0-r12}
	add		sp, sp, #OFFSET_SP

	dsb
	isb

	ldmia 	sp, {sp, lr, pc}^
#endif

el0_sync_handler:

	kernel_entry

	mrs		x0, elr_el1
	str		x0, [sp, #OFFSET_LR]

	mrs		x0, sp_el0
	str		x0, [sp, #OFFSET_SP]

	mrs		x0, spsr_el1
	str		x0, [sp, #OFFSET_PSTATE]

	// Make sure r0 refers to the base of the stack frame
	mov		x0, sp

	str		lr, [sp, #-8]!

	enable_irq
    bl 		trap_handle
    disable_irq

 	ldr		lr, [sp], #8

	// Return value
	str		x0, [sp, #OFFSET_X0]

	ldr		x0, [sp, #OFFSET_LR]
	msr		elr_el1, x0

	ldr 	x0, [sp, #OFFSET_SP]
	msr		sp_el0, x0

	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el1, x0

	kernel_exit

	eret

// Used at entry point of a fork'd process (setting the return value to 0)
ret_from_fork:

#if 0
	mov		r0, #0
	b		__ret_from_fork
#endif

#if 0
.align  5
prefetch_abort:

	// Call the C data abort handler with the following args:
	// r0 = IFAR, r1 = IFSR, r2 = LR

	mrc	p15, 0, r0, c6, c0, 2		// get IFAR
	mrc	p15, 0, r1, c5, c0, 1		// get IFSR
	mov r2, lr

    b __prefetch_abort

.align  5
data_abort:

	// Call the C data abort handler with the following args:
	// r0 = FAR, r1 = FSR, r2 = LR

	mrc	p15, 0, r1, c5, c0, 0		// get FSR
	mrc	p15, 0, r0, c6, c0, 0		// get FAR
	mov r2, lr

    b __data_abort

.align  5
not_used:
    b not_used

#endif

.align  5
el0_irq_handler:

	kernel_entry

	mrs		x0, elr_el1
	str		x0, [sp, #OFFSET_LR]

	mrs		x0, sp_el0
	str		x0, [sp, #OFFSET_SP]

	mrs		x0, spsr_el1
	str		x0, [sp, #OFFSET_PSTATE]

	// Make sure r0 refers to the base of the stack frame
	mov		x0, sp

	str		lr, [sp, #-8]!
    bl 		irq_handle
    ldr		lr, [sp], #8

	ldr		x0, [sp, #OFFSET_LR]
	msr		elr_el1, x0

	ldr 	x0, [sp, #OFFSET_SP]
	msr		sp_el0, x0

	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el1, x0

	kernel_exit

	eret


.align  5
el1_irq_handler:

	kernel_entry

	mrs		x0, spsr_el1
	str		x0, [sp, #OFFSET_PSTATE]

	// Make sure r0 refers to the base of the stack frame
	mov		x0, sp

	str		lr, [sp, #-8]!
    bl 		irq_handle
	ldr		lr, [sp], #8

	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el1, x0

  	kernel_exit

	eret


#if 0
.align  5
fiq:
    b  fiq
#endif


.LCcurrent:
	.quad current_thread

.LClog:
	.quad 0


