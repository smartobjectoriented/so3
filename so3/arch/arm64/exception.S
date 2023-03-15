/*
 * Copyright (C) 2021 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifdef CONFIG_ARM64VT

.global cpu_entrypoint

#else /* CONFIG_ARM64VT */

.extern do_upcall
.globl  pseudo_usr_mode

#endif /* !ARM64VT */

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

.align 11
ENTRY(__vectors)

	// Current EL with SP0 / Synchronous
	.align 7

	mov		x0, lr
	b 	trap_handle_error

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
	mov 	x0, lr
	b		trap_handle_error

	// Current EL with SPx / IRQ
	.align 7

#ifdef CONFIG_AVZ

#ifdef CONFIG_ARM64VT
	b 		el1_2_12_irq_handler
#else
	b 		avz_el01_1_irq_handler
#endif /* CONFIG_ARM64VT */

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
#ifdef CONFIG_ARM64VT
	b 		el12_sync_handler
#else
	b 		el01_sync_handler
#endif

	// Lower EL using AArch64 / IRQ
	.align 7
#ifdef CONFIG_AVZ

#ifdef CONFIG_ARM64VT
	b 		el1_2_12_irq_handler
#else
	b 		avz_el01_1_irq_handler
#endif

#else
	b 		el0_irq_handler
#endif

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
	b 	trap_handle_error

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

#ifdef CONFIG_ARM64VT

#ifdef CONFIG_CPU_PSCI
ENTRY(pre_ret_to_el1)

	wfi

	ldr 	x0, cpu_entrypoint
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

 	mov 	x0, sp
	bl		trap_handle

	str		x0, [sp, #OFFSET_X0]

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

#ifdef CONFIG_ARM64VT
	mrs		x0, elr_el2
	str		x0, [sp, #OFFSET_PC]

	mrs		x0, spsr_el2
	str		x0, [sp, #OFFSET_PSTATE]

	mrs		x0, sp_el1
	str		x0, [sp, #OFFSET_SP]
#else
	mrs		x0, elr_el1
	str		x0, [sp, #OFFSET_PC]

	mrs		x0, spsr_el1
	str		x0, [sp, #OFFSET_PSTATE]
#endif

	// Make sure r0 refers to the base of the stack frame
	mov		x0, sp

	// The stack must stay 16-byte aligned

	str		lr, [sp, #-16]!
    bl 		irq_handle
	ldr		lr, [sp], #16

#ifdef CONFIG_ARM64VT
	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el2, x0

	ldr		x0, [sp, #OFFSET_PC]
	msr		elr_el2, x0

	ldr		x0, [sp, #OFFSET_SP]
	msr		sp_el1, x0
#else
	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el1, x0

	ldr		x0, [sp, #OFFSET_PC]
	msr		elr_el1, x0
#endif

  	kernel_exit

	eret

cpu_entrypoint:
	.quad 0x0

#endif /* CONFIG_ARM64VT */

#ifdef CONFIG_AVZ

/*
 * This function is called at bootstrap and
 * reboot time. It initializes some registers
 */
ENTRY(pre_ret_to_user)

	// Initial state - IRQs off
	disable_irq

	// Get a reference to our domain descriptor
	curdom	x10, x11

#ifdef CONFIG_ARM64VT

	ldr		x2, [sp, #OFFSET_PC]  // Entry point of the guest
	msr		elr_el2, x2

	// Set the CPU in EL1 mode to proceed with
	// the bootstrap of the domain

	mov		x2, #PSR_MODE_EL1t
	msr		spsr_el2, x2

	ldr		x0, [sp, #OFFSET_X21] // Device tree (fdt_addr)

	// According to boot protocol
	mov		x1, #0
	mov		x2, #0
	mov		x3, #0

	// Ready to jump into the agency domain...

	eret

#else /* CONFIG_ARM64VT */

	ldr		x11, [x10, #OFFSET_AVZ_SHARED]
	str		xzr, [x11, #OFFSET_HYPERVISOR_CALLBACK]

	current_cpu 	x11

	// Switch to the guest stack
	ldr 	x0, .LChypervisor_stack
	mov		x12, sp
	str		x12, [x0, x11, lsl #3]

	// Setting pseudo_usr_mode
	ldr 	x0, .LCpseudo_usr_mode
	mov		x1, #1
	str		x1, [x0, x11, lsl #3]

	ldr		x2, [sp, #OFFSET_PC]  // Entry point of the guest
	ldr		x21, [sp, #OFFSET_X21] // Device tree (fdt_addr)
	ldr		x22, [sp, #OFFSET_X22] // Address of start_info

	// Ready to jump into the Linux domain...
	blr		x2

#endif /* !CONFIG_ARM64VT */

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

#ifndef CONFIG_ARM64VT

.align  5
avz_el01_1_irq_handler:

	kernel_entry

	mrs		x0, spsr_el1
	str		x0, [sp, #OFFSET_PSTATE]

	mrs		x0, sp_el0
	str		x0, [sp, #OFFSET_SP]

	mrs		x0, elr_el1
	str		x0, [sp, #OFFSET_PC]

	// Now process the IRQ differentiating execution according to the source (dom/hyp)
	// for managing the stack correctly

	current_cpu	x10

	ldr		x0, .LCpseudo_usr_mode
	ldr		x1, [x0, x10, lsl #3]
	cmp		x1, #0  				// svc ?

	b.eq	svc_path

	mov		x1, #0 					// setting svc
	str		x1, [x0, x10, lsl #3]

	mov		x1, sp
	ldr 	x0, .LChypervisor_stack // Get the running hypervisor SVC stack
	ldr		x0, [x0, x10, lsl #3]

	mov		sp, x0

	curdom	x0, x2
	str		x1, [x0, #OFFSET_G_SP]

	// Make sure r0 refers to the base of the stack frame
	mov		x0, sp

	str		lr, [sp, #-16]!
    bl 		irq_handle
	ldr		lr, [sp], #16

	curdom	x10, x11
	ldr		x11, [x10, #OFFSET_AVZ_SHARED]

	// If the softirq handling leads to trigger an interrupt in the guest,
	// it will be processed by do_evtchn_do_upcall. The way how to
	// process an interrupt with potentially IRQs off is under the
	// responsibility of the guest

	// are some IRQs pending?
	ldrb	w12, [x11, #OFFSET_EVTCHN_UPCALL_PENDING]
	tst		w12, #0xff

	b.eq	restore

upcall_path:

	current_cpu x11

	curdom  x10, x12
	ldr		x12, [x10, #OFFSET_AVZ_SHARED]
	ldr		lr, [x12, #OFFSET_HYPERVISOR_CALLBACK]

	cmp		lr, #0
	beq		restore

	ldr		x0, .LChypervisor_stack   	// running L1 hypervisor stack
	mov		x12, sp
	str		x12, [x0, x11, lsl #3]

	// Get guest stack
	ldr		x12, [x10, #OFFSET_G_SP]
	mov		sp, x12

	// setting pseudo_usr_mode / x0, x1 re-assigned right after
	ldr 	x0, .LCpseudo_usr_mode
	mov		x1, #1
	str		x1, [x0, x11, lsl #3]

	// x0 contains a reference to the stack pointer
	mov		x0, sp

	// Go up to the guest for evtchn processing
	br		lr

restore:

	current_cpu x11

	// Setting pseudo_usr_mode / x0, x1 re-assigned right after
	ldr 	x0, .LCpseudo_usr_mode
	mov		x1, #1
	str		x1, [x0, x11, lsl #3]

	// Restore saved registers

	ldr		x0, .LChypervisor_stack   	// running SVC hypervisor stack
	mov		x10, sp
	str		x10, [x0, x11, lsl #3]

	curdom	x10, x11

	// Get guest stack
	ldr		x11, [x10, #OFFSET_G_SP]
	mov		sp, x11

out_irq:

	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el1, x0

	ldr		x0, [sp, #OFFSET_SP]
	msr		sp_el0, x0

	ldr		x0, [sp, #OFFSET_PC]
	msr		elr_el1, x0

  	kernel_exit

	eret


.align 5

avz_el0_sync_handler:

	kernel_entry

	mrs		x0, elr_el1
	str		x0, [sp, #OFFSET_PC]

	mrs		x0, sp_el0
	str		x0, [sp, #OFFSET_SP]

	mrs		x0, spsr_el1
	str		x0, [sp, #OFFSET_PSTATE]

	current_cpu	x10

	mov		x1, sp
	ldr 	x0, .LChypervisor_stack // Get the running hypervisor SVC stack
	ldr		x0, [x0, x10, lsl #3]

	mov		sp, x0
	curdom  x10, x12
	ldr		x12, [x10, #OFFSET_AVZ_SHARED]
	ldr		x10, [x12, #OFFSET_TRAPS_CALLBACK]

	// Come back on the domain stack
	mov		sp, x1

	cmp		x10, #0
	b.eq	out_sync

	// Make sure r0 refers to the base of the stack frame
	mov		x0, sp

    blr		x10

out_sync:

	ldr		x0, [sp, #OFFSET_PC]
	msr		elr_el1, x0

	ldr 	x0, [sp, #OFFSET_SP]
	msr		sp_el0, x0

	ldr		x0, [sp, #OFFSET_PSTATE]
	msr		spsr_el1, x0

	kernel_exit

	eret

/*
 * The following function is used to restore the migrated domain.
 * Indeed, the receiver environment has not saved anything on its stack regarding
 * a context switch. We can not pursue on right-after-context-switch in the schedule function!
 * But we do not start from boot either. So, we have an appropriate restore glue code to perform
 * an upcall in the newly migrated ME. A first timer IRQ has been set in domain_migration_restore() to
 * avoid a problem in the guest when testing for upcall pending.
 *
 *
 */
ENTRY(after_migrate_to_user)

	b upcall_path


// Hypervisor stack is used for the *current* (running) vcpu svc stack address
svc_path:

	mov		x0, sp

	// The stack must stay 16-byte aligned

	str		lr, [sp, #-16]!
    bl 		irq_handle
	ldr		lr, [sp], #16

    b 		out_irq

pseudo_usr_mode:
	.space CONFIG_NR_CPUS * 8

.LCpseudo_usr_mode:
	.quad	pseudo_usr_mode


#endif /* !CONFIG_ARM64VT */

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

	ldr 	x0, [sp, #OFFSET_SP]
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

	ldr 	x0, [sp, #OFFSET_SP]
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

#ifdef CONFIG_SO3VIRT

.align 5
/*
 * Hypercall trampoline has the following arguments:
 * - x0: hypercall number
 * - x1: first arg (-> r0)
 * - x2: second arg
 * - x3: third arg
 * - x4: fourth arg
 * - x5: work register
 * - x7: hypecall number (ABI)
 *
 * - x8 will contain 0 if no pending evtchn, 1 otherwise
 * - x9 will contain the PSTATE at the hypercall entry and exit
 */
ENTRY(hypercall_trampoline)

    // Stack alignment must stay on 16 bytes.
    sub     sp, sp, #12*8

    stp 	x1, x2, [sp, #16 * 0]
    stp 	x3, x4, [sp, #16 * 1]
    stp		x5, x6, [sp, #16 * 2]
    stp		x7, x8, [sp, #16 * 3]
    stp		x9, x10, [sp, #16 * 4]
    stp		x11, lr, [sp, #16 * 5]

    mov 	x7, x0
    mov 	x0, x1
    mov 	x1, x2
    mov 	x2, x3
    mov 	x3, x4
    mov 	x4, x5

    ldr 	x5, =HYPERVISOR_hypercall_addr
	ldr		x5, [x5]

    blr		x5

	cmp 	x8, #0
	b.eq	out_upcall

	kernel_entry

	// Update PSTATE accordingly
	str		x9, [sp, #OFFSET_PSTATE]

	// Set x0 to sp
	mov		x0, sp
	bl 		irq_handle
	kernel_exit

	// Restore original PSTATE
	msr		daif, x9

out_upcall:

    ldp 	x1, x2, [sp, #16 * 0]
    ldp 	x3, x4, [sp, #16 * 1]
    ldp		x5, x6, [sp, #16 * 2]
    ldp		x7, x8, [sp, #16 * 3]
    ldp		x9, x10, [sp, #16 * 4]
    ldp		x11, lr, [sp, #16 * 5]

    add     sp, sp, #12*8

    ret

#endif /* CONFIG_SO3VIRT */

#if !defined(CONFIG_AVZ) && defined(CONFIG_SOO)


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
