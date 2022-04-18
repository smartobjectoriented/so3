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

#include <thread.h>

/**
 * Set the CPU registers with thread related information
 *
 * @param tcb
 */
void arch_prepare_cpu_regs(tcb_t *tcb) {

	tcb->cpu_regs.x19 = (unsigned long) tcb->th_fn;
	tcb->cpu_regs.x20 = (unsigned long) tcb->th_arg; /* First argument */

	if (tcb->pcb)
		tcb->cpu_regs.x21 = get_user_stack_top(tcb->pcb, tcb->pcb_stack_slotID);
}

/**
 * Give the base address of arguments. This is the top address of the user space.
 * Weird, the user space can be up to bits[46:0]. Use of bit 47 in the virtual address
 * in EL0 leads to a translation fault.
 *
 * @return Base address of arguments
 *
 */
addr_t arch_get_args_base(void) {
	return (addr_t) (USER_STACK_TOP_VADDR - PAGE_SIZE);
}
