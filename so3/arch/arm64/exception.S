/*
 * Copyright (C) 2021-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <thread.h>
#include <syscall.h>

#include <asm/processor.h>

#ifdef CONFIG_AVZ

#include <avz/uapi/avz.h>

#include <avz/domain.h>

.global cpu_entrypoints

#endif /* CONFIG_AVZ */

.globl ret_from_fork
.global __call_sig_handler

.extern current_thread
.extern __sync_serror
.extern avz_handle_el1_serror
.extern avz_handle_el2_serror
.extern do_exit
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

	b	__start				/* ARM64 Image header code0 / reset vector */
	.long	0				/* code1: reserved */
#ifdef CONFIG_VERDIN_IMX8MP
	.quad	LOAD_OFFSET			/* text_offset: DRAM_base + LOAD_OFFSET = load address */
	.quad	0x800000			/* image_size: non-zero → U-Boot reads text_offset */
#else
	.quad	0				/* text_offset: U-Boot uses hardcoded 0x80000 */
	.quad	0				/* image_size: 0 → U-Boot uses hardcoded defaults */
#endif
	.quad	0				/* flags: reserved */
	.quad	0				/* reserved */
	.quad	0				/* reserved */
	.quad	0				/* reserved */
	.ascii	"ARM\x64"			/* magic = 0x644d5241 at offset 56 */
	.long	0				/* reserved */

.align 12
ENTRY(__vectors)

	// Current EL with SP0 / Synchronous
	.align 7

	mov	x0, lr
	b 	trap_handle_error

	// Current EL with SP0 / IRQ
	.align 7

	mov	x0, lr
	b 	trap_handle_error

	// Current EL with SP0 / FIQ
	.align 7

	mov	x0, lr
	b 	trap_handle_error

	// Current EL with SP0 / SError
	.align 7

	mov	x0, lr
	b 	trap_handle_error

	// Current EL with SPx / Synchronous
	.align 7

	mov	x0, lr
	b 	trap_handle_error

	// Current EL with SPx / IRQ
	.align 7

#ifdef CONFIG_AVZ
	b 	el12_2_irq_handler
#else /* CONFIG_AVZ */
	b 	el01_1_irq_handler
#endif /* !CONFIG_AVZ */

	// Current EL with SPx / FIQ
	.align 7

	mov	x0, lr
	b 	trap_handle_error

	// Current EL with SPx / SError
	.align 7

#ifdef CONFIG_AVZ
	b	avz_el2_serror_handler
#else
	/* This slot MUST be emitted in every build: omitting it (e.g. behind
	 * a bare #ifdef CONFIG_AVZ) shifts every subsequent Lower-EL vector up
	 * by 0x80, so an EL0 `svc` lands in the IRQ handler instead of
	 * el01_sync_handler and no syscall is ever serviced. */
	mov	x0, lr
	mrs	x1, esr_el1
	b	__sync_serror
#endif /* CONFIG_AVZ */

	// Lower EL using AArch64 / Synchronous
	.align 7

	// This vector is concerned with the syscall interrupt.

#ifdef CONFIG_AVZ
	b 	el12_sync_handler
#else
	b 	el01_sync_handler
#endif

	// Lower EL using AArch64 / IRQ
	.align 7

#ifdef CONFIG_AVZ
	b 	el12_2_irq_handler
#else /* CONFIG_AVZ */
	b 	el01_1_irq_handler
#endif /* !CONFIG_AVZ */

	// Lower EL using AArch64 / FIQ
	.align 7

	mov	x0, lr
	b 	trap_handle_error

	// Lower EL using AArch64 / SError
	.align 7

#ifdef CONFIG_AVZ
	b	el1_serror_handler
#else
	mov	x0, lr
	b 	trap_handle_error
#endif

	// Lower EL using AArch32 / Synchronous
	.align 7

	mov	x0, lr
	b 	trap_handle_error

	// Lower EL using AArch32 / IRQ
	.align 7

	mov	x0, lr
	b 	trap_handle_error

	// Lower EL using AArch32 / FIQ
	.align 7

	mov	x0, lr
	b 	trap_handle_error

	// Lower EL using AArch32 / SError
	.align 7

	mov	x0, lr
	b 	trap_handle_error


#ifdef CONFIG_CPU_SPIN_TABLE

ENTRY(pre_ret_to_el1_spin)

	mov		x1, x0
	str 	xzr, [x1]
1:
	wfe
 	ldr 	x0, [x1]

    cbz 	x0, 1b

    // Branch to the given address
	msr	elr_el2, x0

	// Set the CPU in EL1 mode to proceed with
	// the bootstrap of the domain

	mov	x2, #PSR_MODE_EL1h

	// Make sure no interrupt coming from CPU #0 is
	// interferring with other CPU bootstrap
	orr	x2, x2, #PSR_I_BIT

	msr	spsr_el2, x2

	// According to boot protocol
	mov	x1, #0
	mov	x1, #0
	mov	x2, #0
	mov	x3, #0

	// Ready to jump into the Agency domain...

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

	/*
	 * Bao-inspired per-CPU entrypoint pattern:
	 * Each secondary CPU reads its own slot in cpu_entrypoints[] indexed
	 * by the Aff0 field of MPIDR_EL1.  CPU0 writes only the target CPU's
	 * slot before sending a targeted IPI via smp_trigger_event(), so no
	 * atomic operations are needed and no other CPU races for the same slot.
	 */
	mrs	x1, mpidr_el1
	and	x1, x1, #0xff			/* Aff0 = logical CPU index */

	/* pre_ret_debug_wfi(cpu_id) */
	mov	x0, x1
	bl	pre_ret_debug_wfi
	mrs	x1, mpidr_el1
	and	x1, x1, #0xff

	/* Busy-poll on cpu_entrypoints[cpu_id] until CPU0's PSCI_CPU_ON handler
	 * writes the entry point.  Using a spin loop (no WFI) to diagnose
	 * whether WFI wakeup is the bottleneck. */
1:
	dsb	ish
	isb

	adr	x2, cpu_entrypoints
	ldr	x0, [x2, x1, lsl #3]		/* cpu_entrypoints[cpu_id] */
	cbz	x0, 1b				/* not yet written — spin */

	/* pre_ret_debug_woke(cpu_id, ep) — x0=ep, x1=cpu_id */
	mov	x19, x0			/* save ep across the call */
	mov	x20, x1			/* save cpu_id across the call */
	mov	x1, x0
	mov	x0, x20
	bl	pre_ret_debug_woke
	mov	x0, x19			/* restore ep */
	mov	x1, x20			/* restore cpu_id (not needed further) */

	/*
	 * Drain any SError pending from flush_dcache_all() inside
	 * __mmu_switch_vttbr (called by secondary_start_kernel).
	 * On Cortex-A53, DC CISW cache maintenance can leave a deferred
	 * physical SError.  With HCR_EL2.AMO=1 it would otherwise fire
	 * the instant we ERET to EL1, hitting el1_serror_handler before
	 * Linux is initialised.  Force it here at EL2 instead so it is
	 * consumed by avz_el2_serror_handler and the EL1 entry is clean.
	 *
	 * NOTE: the SError handler overwrites ELR_EL2, so we must set
	 * ELR_EL2 AFTER the drain.  x0 (entrypoint) is preserved through
	 * kernel_entry/kernel_exit used by avz_el2_serror_handler.
	 */
	msr	daifclr, #4			/* unmask SErrors at EL2 */
	isb					/* pending SError fires → avz_el2_serror_handler */
	msr	daifset, #4			/* re-mask */
	isb

	/* Set ELR_EL2 after the drain so the SError handler cannot clobber it */
	msr	elr_el2, x0

	/* EL1h mode, all exceptions masked on EL1 entry */
	mov	x2, #(PSR_MODE_EL1h | PSR_F_BIT | PSR_I_BIT | PSR_A_BIT | PSR_D_BIT)
	msr	spsr_el2, x2

	/* Initialise the virtual CPU interface for this secondary CPU so it
	 * can receive vIRQs (timer PPI 27, IPIs, SPIs).  Without this, the
	 * vGIC on secondaries stays disabled (HCR.EN=0) and Linux ends up
	 * with no scheduler tick, hitting an RCU stall a few seconds in.
	 * gich_secondary_init() is a leaf call that preserves callee-saved
	 * registers (x19=ep) but clobbers x0-x18. */
	bl	gich_secondary_init

	/* Disable EL1 physical timer (CNTP) to clear any stale U-Boot compare
	 * value; prevents spurious id=27 storm before Linux programs its own CVAL. */
	msr	cntp_ctl_el0, xzr

	/* ARM64 PSCI secondary boot protocol: x0–x3 = 0 */
	mov	x0, #0
	mov	x1, #0
	mov	x2, #0
	mov	x3, #0

	/* Ready to jump into the Linux domain... */

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

// Enter macro to jump into EL2 from EL0 or from EL1
.macro prepare_to_enter_to_el2
	mrs	x0, elr_el2
	str	x0, [sp, #OFFSET_PC]

	mrs	x0, sp_el1
	str	x0, [sp, #OFFSET_SP_USR]

	mrs	x0, spsr_el2
	str	x0, [sp, #OFFSET_PSTATE]

	// Save EL0 register. Use OFFSET_SP as it is not used on the stack frame.
	mrs	x0, sp_el0
	str	x0, [sp, #OFFSET_SP]
	mrs	x0, tpidr_el0
	str	x0, [sp, #OFFSET_TLS_USR]

	// Save EL1 exception register as they may not yet be saved.
	mrs	x0, spsr_el1
	str	x0, [sp, #OFFSET_SPSR_EL1]
	mrs	x0, elr_el1
	str	x0, [sp, #OFFSET_ELR_EL1]

.endm


// Exit macro at the end of an exception routine
// It restores the sp_el0 as well.
.macro prepare_to_exit_to_el1
	ldr	x0, [sp, #OFFSET_PC]
	msr	elr_el2, x0

	ldr 	x0, [sp, #OFFSET_SP_USR]
	msr	sp_el1, x0

	ldr	x0, [sp, #OFFSET_PSTATE]
	msr	spsr_el2, x0

	// Restore EL0 register
	ldr	x0, [sp, #OFFSET_SP]
	msr	sp_el0, x0
	ldr	x0, [sp, #OFFSET_TLS_USR]
	msr	tpidr_el0, x0

	// Restore EL1 exception register as they may not yet be saved.
	ldr	x0, [sp, #OFFSET_SPSR_EL1]
	msr	spsr_el1, x0
	ldr	x0, [sp, #OFFSET_ELR_EL1]
	msr	elr_el1, x0
.endm

.align 5
el12_sync_handler:

	kernel_entry
	prepare_to_enter_to_el2

 	mov 	x0, sp
	bl	trap_handle

	ldr	x0, [sp, #OFFSET_X0]
	ldr	x1, =AVZ_HYPERCALL_SIGRETURN

	cmp	x0, x1
	bne	__nosigreturn

	// Reset the stack frame by removing the one issued from sigreturn
	add	sp, sp, #S_FRAME_SIZE

__nosigreturn:
	prepare_to_exit_to_el1
	kernel_exit

	eret


.align  5
el12_2_irq_handler:

	kernel_entry
	prepare_to_enter_to_el2

	// Make sure r0 refers to the base of the stack frame
	mov	x0, sp
#ifdef CONFIG_GIC_V3
	bl	avz_el2_irq_handle
#else
    	bl 	irq_handle
#endif

	prepare_to_exit_to_el1
  	kernel_exit

	eret

#ifdef CONFIG_AVZ
/*
 * SError while AVZ itself is running at EL2 (Current EL with SPx).
 * Save registers, log ESR_EL2/ELR_EL2, and ERET to resume the
 * interrupted EL2 code (e.g. the scheduler).
 */
.align 5
avz_el2_serror_handler:

	kernel_entry

	mov	x0, sp
	bl	avz_handle_el2_serror

	kernel_exit

	eret

	dsb nsh
	isb

	nop
	nop
	nop
#endif /* CONFIG_AVZ */

.align 5
el1_serror_handler:

	kernel_entry
	prepare_to_enter_to_el2

	mov 	x0, sp
	bl	avz_handle_el1_serror

	prepare_to_exit_to_el1
	kernel_exit

	eret

	dsb nsh
	isb

	nop
	nop
	nop

/* Per-CPU PSCI entry points — indexed by Aff0 of MPIDR_EL1 */
cpu_entrypoints:
	.quad 0x0, 0x0, 0x0, 0x0

__prepare_to_sig_el1_handler:

	/* Preserve the SP_EL2 in the new stack frame used by
	 * the signal handler.
	 */

	ldr	x0, [sp, #OFFSET_SP_USR]
	str	x0, [sp, #(OFFSET_SP_USR - S_FRAME_SIZE)]

	ldr	x0, [sp, #OFFSET_PSTATE]
	str	x0, [sp, #(OFFSET_PSTATE - S_FRAME_SIZE)]

	// Build a new stack frame based on the current
	sub	sp, sp, #S_FRAME_SIZE

	// Set the handler to the PC
	str	x21, [sp, #OFFSET_PC]

	ret

/*
 * This glue code will be called to prepare a resuming
 * of a capsule.
 */
ENTRY(resume_to_guest)

	bl 	__prepare_to_sig_el1_handler

	ldr	x0, [sp, #OFFSET_PSTATE]
	msr	spsr_el2, x0

	ldr	x0, [sp, #OFFSET_PC]
	msr	elr_el2, x0

	ldr	x0, [sp, #OFFSET_SP_USR]
	msr	sp_el1, x0

  	kernel_exit

	eret

	/*
	 * Mitigate Straight-line Speculation.
	 * Guard against Speculating past an ERET instruction and
	 * potentially perform speculative accesses to memory before
	 * processing the exception return
	 */
	dsb nsh
	isb

999:
	b 999b

/*
 * This function is called at bootstrap and
 * reboot time. It initializes some registers
 */
ENTRY(pre_ret_to_user)

	// Initial state - IRQs off
	disable_irq

	ldr	x2, [sp, #OFFSET_LR]  // Entry point of the guest
	msr	elr_el2, x2

	/*
	 * The MMU must be disabled so that the guest can keep its initial boot code.
	 * Make sure CPU EL1 has MMU disabled.
	 */

	mrs	x2, sctlr_el1
	bic	x2, x2, #SCTLR_ELx_M
	msr	sctlr_el1, x2

	// Set the CPU in EL1 mode to proceed with
	// the bootstrap of the domain

	mov	x2, #(PSR_MODE_EL1h | PSR_F_BIT	| PSR_I_BIT | PSR_A_BIT | PSR_D_BIT)

	msr	spsr_el2, x2

	kernel_exit

	// Ensure all page-table writes are visible to the hardware table walker
	// before handing off to the guest at EL1.
	dsb	sy
	isb

	/* Disable EL1 physical timer (CNTP) to clear stale U-Boot compare value. */
	msr	cntp_ctl_el0, xzr

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

999:
	b 999b

#endif /* CONFIG_AVZ */

__prepare_sig_handler:

	/* Preserve the SP_EL1 and the TLS in the new stack frame used
	 * by the signal handler
	 */

	ldr	x2, [sp, #OFFSET_SP_USR]
	str	x2, [sp, #(OFFSET_SP_USR - S_FRAME_SIZE)]

	ldr	x2, [sp, #OFFSET_TLS_USR]
	str	x2, [sp, #(OFFSET_TLS_USR - S_FRAME_SIZE)]

	// Build a new stack frame based on the current
	sub	sp, sp, #S_FRAME_SIZE

	mov	x2, #PSR_MODE_EL0t
	bic	x2, x2, #PSR_I_BIT
	str	x2, [sp, #OFFSET_PSTATE]

	ldr	x1, [x0, #OFFSET_SYS_SIGNUM]
	str	x1, [sp, #OFFSET_X0]

	// Set the handler to the PC
	ldr	x1, [x0, #OFFSET_SYS_SA]
	ldr	x2, [x1, #OFFSET_SA_HANDLER]
	str	x2, [sp, #OFFSET_PC]

	// Set restorer to LR so the handler return to it.
	ldr	x2, [x1, #OFFSET_SA_RESTORER]
	str	x2, [sp, #OFFSET_LR]

	ret

// x0 will be altered
.macro check_pending_signal

	//The return value will be the __sigaction_t pointer or 0 (NULL) if no signal pending
	bl	sig_check

	cmp	x0, #0
	b.eq	1f

	// Configure a stack frame to manage the user handler
	bl	__prepare_sig_handler
1:

.endm

// Enter macro to jump into EL1 from EL0 *or* from EL1
.macro prepare_to_enter_to_el1
	mrs	x0, elr_el1
	str	x0, [sp, #OFFSET_PC]

	mrs	x0, sp_el0
	str	x0, [sp, #OFFSET_SP_USR]

	mrs	x0, tpidr_el0
	str	x0, [sp, #OFFSET_TLS_USR]

	mrs	x0, spsr_el1
	str	x0, [sp, #OFFSET_PSTATE]
.endm


// Exit macro at the end of an exception routine
// It restores the sp_el0 as well.
.macro prepare_to_exit_to_el0
	ldr	x0, [sp, #OFFSET_PC]
	msr	elr_el1, x0

	ldr 	x0, [sp, #OFFSET_SP_USR]
	msr	sp_el0, x0

	ldr	x0, [sp, #OFFSET_TLS_USR]
	msr	tpidr_el0, x0

	ldr	x0, [sp, #OFFSET_PSTATE]
	msr	spsr_el1, x0
.endm


/* Syscalls */
el01_sync_handler:

	kernel_entry
	prepare_to_enter_to_el1

	// Make sure x0 refers to the base of the stack frame
	mov	x0, sp
   	bl 	trap_handle

#ifdef CONFIG_IPC_SIGNAL
	// Check if sigreturn has been called. In this case, we
	// clean the stack frame which has been used to manage the user handler.
	cmp	x8, #SYSCALL_rt_sigreturn
	bne	ret_from_fork

	// Reset the stack frame by removing the one issued from sigreturn
	add	sp, sp, #S_FRAME_SIZE
#endif

	// Entry point for new user threads
ret_from_fork:

#ifdef CONFIG_IPC_SIGNAL
	// Is there any pending signals for this process?
	check_pending_signal
#endif /* CONFIG_IPC_SIGNAL */

	prepare_to_exit_to_el0
	kernel_exit

	eret

.align  5
el01_1_irq_handler:

	kernel_entry
	prepare_to_enter_to_el1

	// Make sure r0 refers to the base of the stack frame
	mov	x0, sp
   	bl 	irq_handle

#ifdef CONFIG_IPC_SIGNAL
	// Is there any pending signals for this process?
	check_pending_signal
#endif /* CONFIG_IPC_SIGNAL */

	prepare_to_exit_to_el0
	kernel_exit

	eret

#if !defined(CONFIG_AVZ) && defined(CONFIG_SOO)

.align 5

ENTRY(__avz_hypercall)

	hvc 	#0
	ret

#endif


