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
#include <config.h>

#include <generated/asm-offsets.h>

#include <asm/processor.h>

.globl  pseudo_usr_mode
.globl	hypervisor_stack

/*
 * This function is called at bootstrap and
 * reboot time. It initializes some registers
 */
ENTRY(pre_ret_to_user)

	// Initial state - IRQs off
	disable_irq

	// Get a reference to our domain descriptor
	curdom	x10, x11
	str		xzr, [x10, #OFFSET_HYPERVISOR_CALLBACK]

	current_cpu 	x11

	// Switch to the guest stack
	ldr 	x0, .LChypervisor_stack
	mov		x12, sp
	str		x12, [x0, x11, lsl #3]

	// Setting pseudo_usr_mode
	ldr 	x0, .LCpseudo_usr_mode
	mov		x1, #1
	str		x1, [x0, x11, lsl #3]

	ldr		x2, [sp, #S_PC]  // Entry point of the guest
	ldr		x21, [sp, #S_X21] // Device tree (fdt_addr)
	ldr		x22, [sp, #S_X22] // Address of start_info

	// Ready to jump into the Linux domain...
	blr		x2

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
#if 0
    @ should be enough

    @ We need to set up a correct vector offset in S_CONTEXT

    current_cpu r11

    curdom  r10

	ldr		r0, .LChypervisor_stack   	@ running SVC hypervisor stack
	str		sp, [r0, r11, lsl #2]

	@ get guest stack (already stacked from save_svc_context)
	ldr		sp, [r10, #OFFSET_G_SP]

	mov		r9, #0x18   @ IRQ -> will drive to evtchn_do_upcall() in the guest
    str		r9, [sp, #S_CONTEXT]

	ldr		sp, [r0, r11, lsl #2]

	b do_upcall
#endif

ENTRY(ret_to_user)
#if 0
	disable_irq 					@ ensure IRQs are disabled

	bl	do_softirq

	vcpu	r10
	ldr		r11, [r10, #OFFSET_SHARED_INFO]

	@ If the softirq handling leads to trigger an interrupt in the guest,
	@ it will be processed by do_evtchn_do_upcall. The way how to
	@ process an interrupt with potentially IRQs off is under the
	@ responsibility of the guest

	@ are some IRQs pending?
	ldrb	r12, [r11, #OFFSET_EVTCHN_UPCALL_PENDING]
	tst		r12, #0xff

	beq	restore

	b	do_upcall
#endif

/*
 * Send event to guest domain
 */
ENTRY(do_upcall)
#if 0
	disable_irq

	current_cpu r11

	curdom  r10

	ldr		lr, [r10, #OFFSET_HYPERVISOR_CALLBACK]
	cmp		lr, #0
	beq		restore

	ldr		r0, .LChypervisor_stack   	@ running SVC hypervisor stack
	str		sp, [r0, r11, lsl #2]

	@ get guest stack (already stacked from save_svc_context)
	ldr		sp, [r10, #OFFSET_G_SP]

	@ setting pseudo_usr_mode / r0, r1 re-assigned right after
	ldr 	r0, .LCpseudo_usr_mode
	mov		r1, #1
	str		r1, [r0, r11, lsl #2]

	@ r0 contains a reference to the stack pointer
	mov		r0, sp

	ldr		r1, [sp, #S_R1]

	mov		pc, lr
#endif

ENTRY(restore)
#if 0
	current_cpu r11

	@ setting pseudo_usr_mode / r0, r1 re-assigned right after
	ldr 	r0, .LCpseudo_usr_mode
	mov		r1, #1
	str		r1, [r0, r11, lsl #2]


	@ restore saved registers

	ldr		r0, .LChypervisor_stack   	@ running SVC hypervisor stack
	str		sp, [r0, r11, lsl #2]

	curdom	r10

	@ get guest stack (already stacked from save_svc_context)
	ldr		sp, [r10, #OFFSET_G_SP]

	ldr		r0, [sp, #S_PSR]			@ Check if return is in guest SVC or guest USR
	msr		spsr_cxsf, r0

	and		r0, r0, #PSR_MODE_MASK
	cmp		r0, #PSR_MODE_USR   			@ usr ?
	bne		restore_svc

	ldr     lr, [sp, #S_PC]!                @ Get PC

	ldmdb   sp, {r0 - lr}^                  @ Get calling r0 - lr
	mov     r0, r0
	add     sp, sp, #S_FRAME_SIZE - S_PC

	movs    pc, lr                          @ return & move spsr_svc into cpsr

restore_svc:

	ldmia	sp, {r0 - pc}^		@ load r0 - pc, cpsr
#endif

/*
 * Register switch
 * r0 = previous vcpu, r1 = previous vcpu_guest_context, r2 = next vcpu_guest_context
 * previous and next are guaranteed not to be the same.
 *
 */
ENTRY(__switch_to)

	mov		x10, #(OFFSET_CPU_REGS + OFFSET_X19)
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

	ldp		x19, x20, [x8], #16		// restore callee-saved registers
	ldp		x21, x22, [x8], #16
	ldp		x23, x24, [x8], #16
	ldp		x25, x26, [x8], #16
	ldp		x27, x28, [x8], #16
	ldp		x29, lr, [x8], #16
	ldr		x9, [x8]
	mov		sp, x9

	ret

pseudo_usr_mode:
	.space NR_CPUS * 8

// Hypervisor stack is used for the *current* (running) vcpu svc stack address
hypervisor_stack:
	.space NR_CPUS * 8


.LCpseudo_usr_mode:
	.quad	pseudo_usr_mode

.LChypervisor_stack:
	.quad	hypervisor_stack

