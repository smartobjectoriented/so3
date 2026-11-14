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

#include <common.h>
#include <process.h>
#include <signal.h>
#include <syscall.h>
#include <schedule.h>

#include <device/irq.h>

#include <asm/processor.h>
#include <asm/mmu.h>

#include <generated/syscall_number.h>

void irq_handle(cpu_regs_t *regs);

/* Red zone of the System V ABI, left untouched below the user stack pointer */

#define X86_RED_ZONE 128

static const char *exception_names[32] = {
	"divide error", "debug", "NMI", "breakpoint", "overflow", "bound range", "invalid opcode",
	"device not available", "double fault", "coprocessor overrun", "invalid TSS", "segment not present",
	"stack fault", "general protection", "page fault", "reserved", "x87 FP error", "alignment check",
	"machine check", "SIMD FP error", "virtualization", "control protection",
};

void show_registers(cpu_regs_t *regs)
{
	lprintk("  rip: %016lx  cs: %lx  rflags: %016lx\n", regs->rip, regs->cs, regs->rflags);
	lprintk("  rsp: %016lx  ss: %lx  error: %lx\n", regs->rsp, regs->ss, regs->error_code);
	lprintk("  rax: %016lx  rbx: %016lx  rcx: %016lx\n", regs->rax, regs->rbx, regs->rcx);
	lprintk("  rdx: %016lx  rsi: %016lx  rdi: %016lx\n", regs->rdx, regs->rsi, regs->rdi);
	lprintk("  rbp: %016lx  r8:  %016lx  r9:  %016lx\n", regs->rbp, regs->r8, regs->r9);
	lprintk("  r10: %016lx  r11: %016lx  r12: %016lx\n", regs->r10, regs->r11, regs->r12);
	lprintk("  r13: %016lx  r14: %016lx  r15: %016lx\n", regs->r13, regs->r14, regs->r15);
	lprintk("  cr2: %016lx  cr3: %016lx\n", read_cr2(), read_cr3());
}

/*
 * A fault in user mode terminates the offending process only, so that the
 * shell gets the control back.
 */

static void user_fault(cpu_regs_t *regs)
{
	pcb_t *pcb = current()->pcb;

	printk("[SO3] process \"%s\" (pid %d) got a %s at rip=0x%lx (cr2=0x%lx, error=0x%lx) - terminating it.\n",
	       pcb ? pcb->name : "?", pcb ? pcb->pid : -1, exception_names[regs->vector & 31], regs->rip, read_cr2(),
	       regs->error_code);

	local_irq_enable();
	sys_do_exit_group(128 + SIGSEGV);
}

/**
 * Entry point of all IDT vectors: CPU exceptions and 8259 interrupts.
 *
 * @param regs	Pointer to the stack frame
 */

void trap_handle(cpu_regs_t *regs)
{
	if ((regs->vector >= X86_IRQ_BASE) && (regs->vector < X86_IRQ_BASE + 16)) {
		irq_handle(regs);
		return;
	}

	if (regs->cs & 3)
		user_fault(regs);

	lprintk("### Kernel exception %ld (%s) ###\n", regs->vector,
		regs->vector < 32 && exception_names[regs->vector] ? exception_names[regs->vector] : "?");
	show_registers(regs);

	kernel_panic();
}

/*
 * The syscall number is read back by syscall_handle() through
 * __get_syscall_args_ext(). It is kept in the TCB, since the thread may be
 * preempted as soon as IRQs are on.
 */

void __get_syscall_args_ext(long *syscall_no)
{
	*syscall_no = current()->cpu_regs.rax;
}

/**
 * Syscall dispatcher, with the x86_64 ABI: number in rax, arguments in rdi,
 * rsi, rdx, r10, r8 and r9, result in rax.
 *
 * @param regs	Frame built by x86_syscall_entry
 * @return	1 for rt_sigreturn, whose frame must be dropped, 0 otherwise
 */

long syscall_entry_handle(cpu_regs_t *regs)
{
	syscall_args_t sys_args;
	long nr = regs->rax;
	long ret;

	sys_args.args[0] = regs->rdi;
	sys_args.args[1] = regs->rsi;
	sys_args.args[2] = regs->rdx;
	sys_args.args[3] = regs->r10;
	sys_args.args[4] = regs->r8;
	sys_args.args[5] = regs->r9;

	/* x86_64 passes clone(flags, stack, ptid, ctid, tls), SO3 expects ARM's tls before ctid. */

	if (nr == SYSCALL_clone) {
		sys_args.args[3] = regs->r8;
		sys_args.args[4] = regs->r10;
	}

	current()->cpu_regs.rax = nr;

	local_irq_enable();
	ret = syscall_handle(&sys_args);
	local_irq_disable();

#ifdef CONFIG_IPC_SIGNAL
	if (nr == SYSCALL_rt_sigreturn)
		return 1;
#endif

	regs->rax = ret;

	return 0;
}

#ifdef CONFIG_IPC_SIGNAL

/*
 * Frame which makes the return to user space enter the signal handler:
 * handler(signum) on the user stack, returning to sa_restorer, which
 * issues rt_sigreturn and gets the interrupted frame back.
 */

void x86_setup_sig_frame(cpu_regs_t *frame, cpu_regs_t *interrupted, __sigaction_t *sigaction)
{
	u64 sp;

	sp = ((interrupted->rsp - X86_RED_ZONE) & ~15UL) - 8;
	*(u64 *) sp = (u64) sigaction->sa->sa_restorer;

	*frame = (cpu_regs_t) {
		.rdi = sigaction->signum,
		.rip = (u64) sigaction->sa->sa_handler,
		.cs = USER_CS,
		.rflags = X86_EFLAGS_IF | X86_EFLAGS_FIXED,
		.rsp = sp,
		.ss = USER_DS,
	};
}

#endif /* CONFIG_IPC_SIGNAL */

/*
 * arch_prctl(ARCH_SET_FS) is how musl sets up the thread pointer on x86_64.
 */

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

SYSCALL_DEFINE2(arch_prctl, int, code, unsigned long, addr)
{
	switch (code) {
	case ARCH_SET_FS:
		if (!user_space_vaddr(addr))
			return -EPERM;

		current()->cpu_regs.fs_base = addr;
		wrmsr(MSR_FS_BASE, addr);
		return 0;

	case ARCH_GET_FS:
		*(unsigned long *) addr = rdmsr(MSR_FS_BASE);
		return 0;

	default:
		return -EINVAL;
	}
}
