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
#include <memory.h>

/**
 * Set the CPU registers with thread related information
 *
 * @param tcb
 */
void arch_prepare_cpu_regs(tcb_t *tcb) {

	tcb->cpu_regs.r4 = (unsigned long) tcb->th_fn;
	tcb->cpu_regs.r5 = (unsigned long) tcb->th_arg; /* First argument */

	if (tcb->pcb)
		tcb->cpu_regs.r6 = get_user_stack_top(tcb->pcb, tcb->pcb_stack_slotID);
}

/**
 * The page of arguments related to a process is located on top of the stack.
 * @return	Base address of arguments
 */
addr_t arch_get_args_base(void) {
	return (CONFIG_KERNEL_VADDR - PAGE_SIZE);
}
