/*
 * Copyright (C) 2022 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <asm/processor.h>
#include <asm/mmu.h>

__switch_context:
	mov		x10, #(OFFSET_TCB_CPU_REGS + OFFSET_X19)
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

// Switch the MMU to a L0 page table
// x0 contains the TTBR related to this CPU for the L0 page table

ENTRY(__mmu_switch)

	dsb   sy                     /* Ensure the flushes happen before continuing */
    isb                          /* Ensure synchronization with previous changes to text */

    // At the hypervisor level, we take care about TTBR1 page table only.
    msr    TTBR1_EL1, x0

    isb

	ret

ENTRY(cpu_do_idle)

	dsb	  sy			// WFI may enter a low-power mode
	wfi

	ret
