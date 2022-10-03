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
#include <syscall.h>

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

void __check_ptrace_syscall(void) {
	/* Update the CPU regs of the TCB belonging to the current thread */
	update_cpu_regs();

	if (current()->pcb->ptrace_pending_req == PTRACE_SYSCALL)
		ptrace_process_stop();
}

/*
 * Implementation of ptrace syscall
 */
int do_ptrace(enum __ptrace_request request, uint32_t pid, void *addr, void *data) {
	pcb_t *pcb;

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

		retrieve_cpu_regs((struct user *) data, pcb);
		break;

	default:
		printk("%s: request %d not yet implemented\n.", __func__, request);
	}

	return 0;
}

