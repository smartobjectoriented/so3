/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <string.h>

#include <asm/processor.h>

void __x86_switch_stack(u64 *prev_ksp, u64 next_ksp);

/* FPU/SSE state of a thread which starts from scratch (fninit + default MXCSR) */

static u8 fpu_init_image[X86_FXSAVE_SIZE] __attribute__((aligned(16)));

/* Called once at boot, after the FPU/SSE units have been enabled */

void x86_fpu_init(void)
{
	u32 mxcsr = 0x1f80;

	asm volatile("fninit\n"
		     "ldmxcsr %1\n"
		     "fxsave64 (%0)\n"
		     :
		     : "r"(fpu_init_image), "m"(mxcsr)
		     : "memory");
}

void x86_fpu_reset(void)
{
	asm volatile("fxrstor64 (%0)" : : "r"(fpu_init_image) : "memory");
}

/*
 * Lay out the initial kernel stack of a thread as __x86_switch_stack()
 * leaves it: return address, six callee-saved registers, then the FXSAVE
 * image followed by the stack pointer to restore.
 */

static void prepare_switch_frame(tcb_t *tcb, addr_t base, addr_t entry, u64 rbx, u64 r12, bool inherit_fpu)
{
	u64 *sp = (u64 *) base;
	u8 *fx;

	*--sp = entry;
	*--sp = 0; /* rbp */
	*--sp = rbx;
	*--sp = r12;
	*--sp = 0; /* r13 */
	*--sp = 0; /* r14 */
	*--sp = 0; /* r15 */

	fx = (u8 *) (((addr_t) sp - X86_FXSAVE_SIZE - 16) & ~15UL);

	if (inherit_fpu)
		asm volatile("fxsave64 (%0)" : : "r"(fx) : "memory");
	else
		memcpy(fx, fpu_init_image, X86_FXSAVE_SIZE);

	*(u64 *) (fx + X86_FXSAVE_SIZE) = (u64) sp;

	tcb->cpu_regs.ksp = (u64) fx;
}

/**
 * Set the CPU registers with thread related information
 *
 * @param tcb Thread to set CPU registers
 * @param args Clone arguments with information about the thread
 */

void arch_prepare_cpu_regs(tcb_t *tcb, clone_args_t *args)
{
	cpu_regs_t *user_regs;

	if (args->pcb == NULL) {
		/* Kernel thread must have a function to call */

		BUG_ON(args->fn == NULL);

		tcb->cpu_regs.fs_base = 0;

		prepare_switch_frame(tcb, get_kernel_stack_top(tcb->stack_slotID), (addr_t) __thread_prologue_kernel,
				     (u64) args->fn, (u64) args->fn_arg, false);
		return;
	}

	user_regs = (cpu_regs_t *) arch_get_kernel_stack_frame(tcb);

	if (args->fn) {
		/* Special userspace case to start root process. */

		BUG_ON(args->stack == 0);
		arch_restart_user_thread(tcb, args->fn, args->stack);

		prepare_switch_frame(tcb, (addr_t) user_regs, (addr_t) ret_from_fork, 0, 0, false);
		return;
	}

	/* fork() or clone(): start from a copy of the caller's user registers */

	*user_regs = *(cpu_regs_t *) arch_get_kernel_stack_frame(current());

	if (args->stack)
		user_regs->rsp = args->stack;

	tcb->cpu_regs.fs_base = (args->flags & CLONE_SETTLS) ? args->tls : rdmsr(MSR_FS_BASE);

	/* Return value 0 for the new thread */

	user_regs->rax = 0;

	prepare_switch_frame(tcb, (addr_t) user_regs, (addr_t) ret_from_fork, 0, 0, true);
}

/**
 * Reset the user registers of a thread to start a new program, in
 * particular after an exec()
 *
 * @param tcb Thread to restart
 * @param fn_entry Entry point of the program
 * @param stack_top Top of the user stack
 */

void arch_restart_user_thread(tcb_t *tcb, th_fn_t fn_entry, addr_t stack_top)
{
	cpu_regs_t *user_regs = (cpu_regs_t *) arch_get_kernel_stack_frame(tcb);

	*user_regs = (cpu_regs_t) {
		.rip = (u64) fn_entry,
		.cs = USER_CS,
		.rflags = X86_EFLAGS_IF | X86_EFLAGS_FIXED,
		.rsp = stack_top,
		.ss = USER_DS,
	};

	tcb->cpu_regs.fs_base = 0;

	if (tcb == current()) {
		wrmsr(MSR_FS_BASE, 0);
		x86_fpu_reset();
	}
}

/**
 * Get the base address of the arguments of a new process
 */

addr_t arch_get_args_base(void)
{
	return (addr_t) (USER_STACK_TOP_VADDR - PAGE_SIZE);
}

/**
 * Get the user registers frame, at the top of the kernel stack
 *
 * @param tcb Thread to get the user registers frame
 */

addr_t arch_get_kernel_stack_frame(tcb_t *tcb)
{
	return get_kernel_stack_top(tcb->stack_slotID) - sizeof(cpu_regs_t);
}

void __switch_to(tcb_t *prev, tcb_t *next)
{
	static u64 boot_ksp;

	if (prev)
		prev->cpu_regs.fs_base = rdmsr(MSR_FS_BASE);

	wrmsr(MSR_FS_BASE, next->cpu_regs.fs_base);

	__x86_switch_stack(prev ? &prev->cpu_regs.ksp : &boot_ksp, next->cpu_regs.ksp);
}
