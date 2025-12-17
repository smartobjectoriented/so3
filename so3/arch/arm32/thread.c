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

#include <process.h>
#include <thread.h>
#include <memory.h>

/**
 * Set the CPU registers with thread related information
 *
 * @param tcb Thread to set registers.
 * @param args Clone arguments with values to set to registers.
 */
void arch_prepare_cpu_regs(tcb_t *tcb, clone_args_t *args)
{
	cpu_regs_t *user_regs;

	if (args->pcb == NULL) {
		/* Kernel thread must have a function to call */
		BUG_ON(args->fn == NULL);
		tcb->cpu_regs.r4 = (unsigned long) args->fn;
		tcb->cpu_regs.r5 = (unsigned long) args->fn_arg;

		tcb->cpu_regs.lr = (unsigned long) __thread_prologue_kernel;
		tcb->cpu_regs.sp = get_kernel_stack_top(tcb->stack_slotID);
	} else {
		user_regs = (cpu_regs_t *) arch_get_kernel_stack_frame(tcb);

		if (args->fn) {
			/* Special userspace case to start root process. */
			BUG_ON(args->stack == 0);
			arch_restart_user_thread(tcb, args->fn, args->stack);
		} else {
			/* Copy userspace registers */
			*user_regs = *(cpu_regs_t *) arch_get_kernel_stack_frame(current());

			/* Normal userspace that will copy userspace registers */
			if (args->stack)
				user_regs->sp_usr = args->stack;

			if (args->flags & CLONE_SETTLS)
				user_regs->tls_usr = args->tls;
		}

		tcb->cpu_regs.lr = (unsigned long) ret_from_fork;
		/* Take into account the user registers frame on the kernel stack */
		tcb->cpu_regs.sp = (addr_t) user_regs;
	}
}

/**
 * Restart a user thread by reseting all registers to 0 and settings entry point and stack.
 *
 * @param tcb Thread to restart.
 * @param fn_entry Userspace entry point of the thread.
 * @param stack_top New top address of the userspace stack.
 */
void arch_restart_user_thread(tcb_t *tcb, th_fn_t fn_entry, addr_t stack_top)
{
	cpu_regs_t *user_regs = (cpu_regs_t *) arch_get_kernel_stack_frame(tcb);

	/* Reset all user's registers to zero except for PC, PSTATE and SP
	 * which needs to be set to the properly start the thread.
	 */
	*user_regs = (cpu_regs_t) {
		.pc = (u32) fn_entry,
		.psr = PSR_USR_MODE,
		.sp_usr = stack_top,
	};
}

/**
 * The page of arguments related to a process is located on top of the stack.
 * @return	Base address of arguments
 */
addr_t arch_get_args_base(void)
{
	return (CONFIG_KERNEL_VADDR - PAGE_SIZE);
}

/**
 * Get the top address of the kernel stack of a user thread with space for registers values.
 *
 * @param tcb Thread to get the stack address.
 */
addr_t arch_get_kernel_stack_frame(tcb_t *tcb)
{
	return get_kernel_stack_top(tcb->stack_slotID) - SVC_STACK_FRAME_SIZE;
}
