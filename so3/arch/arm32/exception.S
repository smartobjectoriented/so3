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

@
@ entry points and handlers for exceptions and interrupts
@
@ based on explanations in "Building Bare Metal ARM Systems with GNU"
@ http://www.state-machine.com/arm/Building_bare-metal_ARM_with_GNU.pdf
@ and on U-Boot implementation

#include <linkage.h>
#include <thread.h>
#include <syscall.h>
#include <asm-offsets.h>

#include <asm/processor.h>

/* use the special section (.vectors.text), to enable fine-tuning
 * of the placement of this section inside the linker script
 */
.section ".vectors.text", "ax"

.globl ret_from_fork
.global __call_sig_handler

.extern current_thread
.extern irq_handle
.extern syscall_handle

.extern dumpstack

.extern __prefetch_abort
.extern __data_abort
.extern __undefined_instruction

.extern do_exit
.extern schedule_isr
.extern __check_ptrace_syscall
.extern sig_check
.extern __stack_alignment_fault

.extern __start

#if 0 /* Debugging purposes */
.extern __log
.extern __mem

.macro MEM t, a, l

	stmfd	sp!, {r0-r12, lr}
	mov		r0, #\t
	mov 	r1, \a
	mov		r2, #\l
	bl		__mem
	ldmfd 	sp!, {r0-r12, lr}
.endm

.macro LOG a, b
	str	\b, .LClog

	stmfd	sp!, {r0-r12, lr}
	mov		r0, #\a
	ldr		r1, .LClog
	bl		__log
	ldmfd 	sp!, {r0-r12, lr}
.endm

#endif /* 0 */

ENTRY(__vectors)
		    b   __start @ reset
            ldr pc, _undefined_instruction
            ldr pc, _syscall_interrupt
            ldr pc, _prefetch_abort
            ldr pc, _data_abort
            ldr pc, _not_used
            ldr pc, _irq
            ldr pc, _fiq

_undefined_instruction: .word undefined_instruction
_syscall_interrupt:     .word syscall_interrupt
_prefetch_abort:        .word prefetch_abort
_data_abort:            .word data_abort
_not_used:              .word not_used
_irq:                   .word irq
_fiq:                   .word fiq
_pad:                   .word 0x12345678 @ now 16*4=64

@ exception handlers
.align  5
undefined_instruction:

    b  __undefined_instruction

@ Prepare to call a handler associated to a pending signal
@ r0 contains the reference to the sigaction_t structure related to the signal to be processed.

__prepare_sig_handler:

	@ Alignment guard
	tst		sp, #0x7	@ 8-bytes aligned
	bne		__stack_alignment_fault

	str		sp, [sp, #(OFFSET_SP-SVC_STACK_FRAME_SIZE)]	@ save sp
	@ Build a new stack frame based on the current
	sub		sp, sp, #SVC_STACK_FRAME_SIZE

	@ Make sure the spsr is with Thumb de-activated to perform normal execution of the handler

	mov		r1, #PSR_USR_MODE		@ Ensure the handler will run in user mode (situation where
								@ the current frame inherits from code running in SVC).

	str		r1, [sp, #OFFSET_PSR]		@ Save the updated SPSR

	@ Set the argument (signum, handler) to r0 & r1
	ldr		r1, [r0, #OFFSET_SYS_SIGNUM]
	str		r1, [sp, #OFFSET_R0]

	ldr 	r0, [r0, #OFFSET_SYS_SA]
	ldr		r1, [r0, #OFFSET_SA_HANDLER]
	str		r1, [sp, #OFFSET_R1]

	ldr		r1, [r0, #OFFSET_SA_RESTORER]
	str		r1, [sp, #OFFSET_PC]		@ Set the handler to the PC

	/* Set the current sp_usr to have a valid stack in the user space */

	ldr 	r0, .LCcurrent
	ldr		r0, [r0]
	ldr 	r0, [r0, #(OFFSET_TCB_CPU_REGS + OFFSET_SP_USR)]
	str		r0, [sp, #OFFSET_SP_USR]

	mov		pc, lr				@ Back to the caller

.macro check_pending_signal
	@ Is there any pending signals for this process?
	bl		sig_check

	cmp		r0, #0
	beq		1f

	@ Configure a stack frame to manage the user handler
	bl		__prepare_sig_handler
1:

.endm

@ IRQs are off
@ ARM EABI: the syscall nr is stored in r7
.align  5
syscall_interrupt:

	@ At the exception entry, the stack must be 8-byte aligned.
	@ If it is not the case (gcc might not respect the AAPCS convention for optimization purposes),
	@ sp will be adjusted. The original sp is preserved and will be correctly restored at the exit.

	tst		sp, #0x7	@ 8-bytes aligned
	strne	sp, [sp, #(OFFSET_SP-SVC_STACK_FRAME_SIZE - 4)]	@ save sp
	subne	sp, sp, #4
	streq	sp, [sp, #(OFFSET_SP-SVC_STACK_FRAME_SIZE)]	@ save sp

	@ Alignment guard
	tst		sp, #0x7		@ 8-bytes aligned
	bne		__stack_alignment_fault

	@ Build the stack frame to store registers

	sub		sp, sp, #SVC_STACK_FRAME_SIZE

	str		lr, [sp, #OFFSET_LR]	@ save lr in lr
	str		lr, [sp, #OFFSET_PC]	@ save lr in pc

	stmia 	sp, {r0-r12}  	@ Store registers

    mrs 	lr, spsr        		@ Get spsr
	str		lr, [sp, #OFFSET_PSR]	@ Store spsr

	@ Saving user mode registers (sp_usr, lr_usr)
	add		lr, sp, #OFFSET_SP_USR
	stmia	lr, {sp, lr}^

	ldr		r0, [sp, #OFFSET_SP_USR]
	ldr 	r1, .LCcurrent
	ldr 	r1, [r1]

	str 	r0, [r1, #(OFFSET_TCB_CPU_REGS + OFFSET_SP_USR)]

	@ Restore r0-r2
	ldmia	sp, {r0-r2}

#ifdef CONFIG_MMU
	@ Give a chance to a ptrace tracer to monitor us (before the syscall)
	stmfd 	sp!, {r0-r4}
	bl		__check_ptrace_syscall
	ldmfd 	sp!, {r0-r4}
#endif

	cpsie   i 			@ Re-enable interrupts
    bl 		syscall_handle
    cpsid	i			@ Re-disable interrupts to be safe in regs manipulation

	@ Check if sigreturn has been called. In this case, we
	@ clean the stack frame which has been used to manage the user handler.
	cmp		r7, #SYSCALL_SIGRETURN
	bne		__no_sigreturn

	@ Reset the stack frame by removing the one issued from sigreturn
	add		sp, sp, #SVC_STACK_FRAME_SIZE

__no_sigreturn:

#ifdef CONFIG_MMU
	@ Give a chance to a ptrace tracer to monitor us (after the syscall)
	stmfd 	sp!, {r0-r4}
	bl	__check_ptrace_syscall
	ldmfd 	sp!, {r0-r4}
#endif

__ret_from_fork:

	@ Store the return value on the stack frame
	cmp		r7, #SYSCALL_SIGRETURN
	strne	r0, [sp, #OFFSET_R0]

#ifdef CONFIG_IPC_SIGNAL
	@ Is there any pending signals for this process?

	check_pending_signal
#endif /* CONFIG_IPC_SIGNAL */

    ldr 	lr, [sp, #OFFSET_PSR]	@ get the saved spsr and adjust the stack pointer
    msr		spsr, lr

	@ Restoring user mode registers (sp_usr, lr_usr)
	add		lr, sp, #OFFSET_SP_USR
	ldmia	lr, {sp, lr}^

 	ldmia 	sp, {r0-r12}
	add		sp, sp, #OFFSET_SP

	dsb
	isb

	ldmia 	sp, {sp, lr, pc}^



@ Used at entry point of a fork'd process (setting the return value to 0)
ret_from_fork:
	mov		r0, #0
	b		__ret_from_fork

.align  5
prefetch_abort:

	@ Call the C data abort handler with the following args:
	@ r0 = IFAR, r1 = IFSR, r2 = LR

	mrc	p15, 0, r0, c6, c0, 2		@ get IFAR
	mrc	p15, 0, r1, c5, c0, 1		@ get IFSR
	mov r2, lr

    b __prefetch_abort

.align  5
data_abort:

	@ Call the C data abort handler with the following args:
	@ r0 = FAR, r1 = FSR, r2 = LR

	mrc	p15, 0, r1, c5, c0, 0		@ get FSR
	mrc	p15, 0, r0, c6, c0, 0		@ get FAR
	mov r2, lr

    b __data_abort

.align  5
not_used:
    b not_used

.align  5
irq:

    @ IRQ mode (lr_irq, sp_irq, cpsr_irq)

    @ Store original r0, and lr_irq in the IRQ stack sp = sp_irq
    @ sp_irq is a simple array declared in arch/arm/setup.c (so address can be ascending)

    str 	r0, [sp]		@ original r0
    str 	lr, [sp, #4]	@ lr_irq

    mrs 	r0, spsr 		@ to preserve irq bit
    str		r0, [sp, #8]	@ spsr_irq

    mov		r0, sp			@ to maintain a reference on sp_irq; r0 will not be affected after mode switch

    @ Now switch back to SVC. IRQs are disabled
    mrs		lr, cpsr

    bic		lr, lr, #PSR_MODE_MASK
    orr		lr, lr, #PSR_SVC_MODE

    @ switch to SVC
    msr		cpsr, lr

    @ --- SVC mode from now on ---

	@ At the exception entry, the stack must be 8-byte aligned.
	@ If it is not the case (gcc might not respect the AAPCS convention for optimization purposes),
	@ sp will be adjusted. The original sp is preserved and will be correctly restored at the exit.

	tst		sp, #0x7	@ 8-bytes aligned
	strne	sp, [sp, #(OFFSET_SP-SVC_STACK_FRAME_SIZE - 4)]	@ save sp
	subne	sp, sp, #4
	streq	sp, [sp, #(OFFSET_SP-SVC_STACK_FRAME_SIZE)]	@ save sp

	@ Alignment guard
	tst		sp, #0x7		@ 8-bytes aligned
	bne		__stack_alignment_fault

	sub		sp, sp, #SVC_STACK_FRAME_SIZE

	@ Store the lr_svc (before the irq)
	str		lr, [sp, #OFFSET_LR]	@ current lr_svc

    @ preserve spsr for future IRQ enabling
    ldr		lr, [r0, #8]   			@ retrieve spsr_irq
    str		lr, [sp, #OFFSET_PSR]	@ location of spsr_svc (hence cpsr right before the interrupt)

	@ Check if it is necessary to preserve sp_usr and lr_usr
	and		lr, lr, #PSR_MODE_MASK
	cmp		lr, #PSR_USR_MODE

	@ Saving user mode registers (sp_usr, lr_usr)
	addeq	lr, sp, #OFFSET_SP_USR
	stmeqia	lr, {sp, lr}^

	@ Retrieve the lr_irq to set the pc out of this routine
	ldr 	lr, [r0, #4]			@ retrieve lr_irq to set lr_svc
	sub 	lr, lr, #4				@ Adjust the lr since it is automatically set from pc (in advance of 2 instructions due to the pipeline)
	str 	lr, [sp, #OFFSET_PC]  	@ future pc at exit

	ldr 	r0, [r0]			    @ original r0

    stmia 	sp, {r0-r12}

	@ cpsr is still up-to-date regarding the comparison against CPU mode.
	ldreq	r0, [sp, #OFFSET_SP_USR]
	ldreq 	r1, .LCcurrent
	ldreq 	r1, [r1]
	streq 	r0, [r1, #(OFFSET_TCB_CPU_REGS + OFFSET_SP_USR)]

	@ Make sure r0 refers to the base of the stack frame
	mov		r0, sp

    bl 		irq_handle

#ifdef CONFIG_IPC_SIGNAL
	@ Is there any pending signals for this process?

	check_pending_signal
#endif /* CONFIG_IPC_SIGNAL */

    ldr 	lr, [sp, #OFFSET_PSR]	@ get the saved spsr and adjust the stack pointer
    msr		spsr, lr

	@ Check if it is necessary to restore sp_usr and lr_usr
	and		lr, lr, #PSR_MODE_MASK
	cmp		lr, #PSR_USR_MODE

	@ Restoring user mode registers (sp_usr, lr_usr)
	addeq	lr, sp, #OFFSET_SP_USR
	ldmeqia	lr, {sp, lr}^

	@ Restore registers
   	ldmia 	sp, {r0-r12}

	add		sp, sp, #OFFSET_SP

	dsb
	isb

    @ Now, we retrieve the final registers, sp will be adjusted automatically
    ldmia 	sp, {sp, lr, pc}^

.align  5
fiq:
    b  fiq

.LCcurrent:
	.word current_thread

.LClog:
	.word 0

