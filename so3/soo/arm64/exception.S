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

