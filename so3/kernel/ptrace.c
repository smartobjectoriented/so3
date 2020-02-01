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

#include <ptrace.h>
#include <process.h>
#include <user.h>

#include <asm/syscall.h>

/*
 * Stop the execution of the process, i.e. its main_thread and all other threads
 * (used by a ptrace activity for example).
 */
void ptrace_process_stop(void) {

	/* Wake up the tracer... */
	/* It may be the case that the parent did not have time to wait for us and is still in ready */
	if (current()->pcb->parent->main_thread->state != THREAD_STATE_READY)
		ready(current()->pcb->parent->main_thread);

	/* The process is in waiting state */
	/* ... and wait on the tracer */

	current()->pcb->state = PROC_STATE_WAITING; /* No further threads of this process may be running */

	waiting();
}


void __check_ptrace_traceme(void) {
	tcb_t *tcb = current();

	/* Check if the process is a tracee and if this thread is the main thread,
	 * therefore we need to be stopped until the tracer executes waitpid()
	 */

	if ((tcb->pcb != NULL) && (tcb == tcb->pcb->main_thread) && (tcb->pcb->ptrace_pending_req == PTRACE_TRACEME))
		ptrace_process_stop();
}

/*
 * Update the CPU registers in the TCB cpu regs structure.
 * These values can then be used by ptrace for instance.
 */
void update_cpu_regs(void) {
	register uint32_t __r0 asm("r0");
	register uint32_t __r1 asm("r1");
	register uint32_t __r2 asm("r2");
	register uint32_t __r3 asm("r3");
	register uint32_t __r4 asm("r4");
	register uint32_t __r5 asm("r5");
	register uint32_t __r6 asm("r6");
	register uint32_t __r7 asm("r7");
	register uint32_t __r8 asm("r8");
	register uint32_t __r9 asm("r9");
	register uint32_t __r10 asm("r10");
	register uint32_t __r11 asm("r11");
	register uint32_t __r12 asm("r12");
	register uint32_t __r13 asm("r13");
	register uint32_t __r14 asm("r14");

	/* Keep the values permanent */
	uint32_t r0 = __r0;
	uint32_t r1 = __r1;
	uint32_t r2 = __r2;
	uint32_t r3 = __r3;
	uint32_t r4 = __r4;
	uint32_t r5 = __r5;
	uint32_t r6 = __r6;
	uint32_t r7 = __r7;
	uint32_t r8 = __r8;
	uint32_t r9 = __r9;
	uint32_t r10 = __r10;
	uint32_t r11 = __r11;
	uint32_t r12 = __r12;
	uint32_t r13 = __r13;
	uint32_t r14 = __r14;


	/* Finally update the tcb structure */
	tcb_t *tcb = current();

	tcb->cpu_regs.r0 = r0;
	tcb->cpu_regs.r1 = r1;
	tcb->cpu_regs.r2 = r2;
	tcb->cpu_regs.r3 = r3;
	tcb->cpu_regs.r4 = r4;
	tcb->cpu_regs.r5 = r5;
	tcb->cpu_regs.r6 = r6;
	tcb->cpu_regs.r7 = r7;
	tcb->cpu_regs.r8 = r8;
	tcb->cpu_regs.r9 = r9;
	tcb->cpu_regs.r10 = r10;
	tcb->cpu_regs.fp = r11;
	tcb->cpu_regs.ip = r12;
	tcb->cpu_regs.sp = r13;
	tcb->cpu_regs.lr = r14;

}

void __check_ptrace_syscall(void) {
	update_cpu_regs();
	if (current()->pcb->ptrace_pending_req == PTRACE_SYSCALL)
		ptrace_process_stop();
}

/*
 * Implementation of ptrace syscall
 */
int do_ptrace(enum __ptrace_request request, uint32_t pid, void *addr, void *data) {
	pcb_t *pcb;
	struct user *uregs;

	switch (request) {
	case PTRACE_TRACEME:
		/* pid not used here */
		current()->pcb->ptrace_pending_req = request;
		break;

	case PTRACE_SYSCALL:
		pcb = find_proc_by_pid(pid);

		/* To set a ptrace request within a child, it must be first waiting, being stopped some where
		 * by a previous ptrace request (the initial is typically PTRACE_TRACEME).
		 * Otherwise, the request is ignored.
		 */
		if (!pcb) {
			set_errno(ESRCH);
			return -1;
		}

		if (pcb->state == PROC_STATE_WAITING)
			pcb->ptrace_pending_req = request;
		break;

	case PTRACE_GETREGS:
		if (!data) {
			set_errno(EFAULT);
			return -1;
		}

		pcb = find_proc_by_pid(pid);

		if (!pcb) {
			set_errno(ESRCH);
			return -1;
		}

		uregs = (struct user *) data;

		uregs->regs.uregs[0] = pcb->main_thread->cpu_regs.r0;
		uregs->regs.uregs[1] = pcb->main_thread->cpu_regs.r1;
		uregs->regs.uregs[2] = pcb->main_thread->cpu_regs.r2;
		uregs->regs.uregs[3] = pcb->main_thread->cpu_regs.r3;
		uregs->regs.uregs[4] = pcb->main_thread->cpu_regs.r4;
		uregs->regs.uregs[5] = pcb->main_thread->cpu_regs.r5;
		uregs->regs.uregs[6] = pcb->main_thread->cpu_regs.r6;
		uregs->regs.uregs[7] = pcb->main_thread->cpu_regs.r7;
		uregs->regs.uregs[8] = pcb->main_thread->cpu_regs.r8;
		uregs->regs.uregs[9] = pcb->main_thread->cpu_regs.r9;
		uregs->regs.uregs[10] = pcb->main_thread->cpu_regs.r10;
		uregs->regs.uregs[11] = pcb->main_thread->cpu_regs.fp;
		uregs->regs.uregs[12] = pcb->main_thread->cpu_regs.ip;
		uregs->regs.uregs[13] = pcb->main_thread->cpu_regs.sp;
		uregs->regs.uregs[14] = pcb->main_thread->cpu_regs.lr;

		break;

	default:
		printk("%s: request %d not yet implemented\n.", __func__, request);
	}

	return 0;
}

