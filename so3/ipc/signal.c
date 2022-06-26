/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2018 David Truan <david.truan@heig-vd.ch>
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

 #if 0
 #define DEBUG
 #endif

#include <signal.h>
#include <schedule.h>
#include <process.h>
#include <thread.h>
#include <types.h>
#include <softirq.h>

#include <asm/processor.h>

void do_sigreturn(void) {

	/* Nothing at the moment ... */

	/* We should actually verify if sigreturn is called by the libc during
	 * a signal handler processing, or whether it is called by the user itself.
	 * The latter case should lead to returning -1. Will be implemented later on.
	 */

	/*
	 * Cleaning the stack frame used for signal handling will be carried out
	 * during along the return path to the user space.
	 */
}


/**
 * Checks if signals were set and executes their handlers if so.
 */
__sigaction_t *sig_check(void) {
	size_t i;

	if (!current() || (current()->pcb == NULL))
		return NULL;

 	for (i = 1; i < _NSIG; i++) {

 		/* Check the signal bit for each signals */
		if (current()->pcb->sigset_map.sigmap[(i-1) / (8*sizeof(long))] & (1UL << (i-1) % (8*sizeof(long)))) {

			/* Check if a handler was specified for this signal */

			if (current()->pcb->sa[i].sa_handler != NULL) {
				current()->pcb->__sa[i].sa = &current()->pcb->sa[i];
				current()->pcb->__sa[i].signum = i;

				current()->pcb->sigset_map.sigmap[(i-1) / (8*sizeof(long))] &= ~(1UL << (i-1) % (8*sizeof(long)));

				return  &current()->pcb->__sa[i];
			}
		}
	}

 	return NULL;
}

#if 0 /* Debugging purposes */
void __log(int i, int j) {

	lprintk("## LOG %d %x\n", i, j);
}

static char stack[100];
static char *addr = NULL;
void __mem(int a, char *adr, int log) {

	int i;
	if ((addr == NULL) && (a == 0))
		return ;

	lprintk("## test mem %d\n", log);
	if (a == 1) {
		for (i = 0; i < 100; i++)
			stack[i] = *(adr+i);
		addr = adr;
	} else {
		for (i = 0; i < 100; i++)
			if (stack[i] != *(addr+i))
				lprintk("################ MEM DIFF %d old: %x  new: %x !!\n", i, stack[i], *(addr+i));


	}
}
#endif /* 0 */

int do_sigaction(int signum, const sigaction_t *action, sigaction_t *old_action)
{
	if (signum < 0 || signum >= _NSIG) {
		DBG("signum not valid!\n");
		return -1;
	}

	if (old_action != NULL)
		*old_action = current()->pcb->sa[signum];

	/* Copy action into the process sigaction_t array */
	if (action) {
		if (action->sa_handler == SIG_IGN)
			current()->pcb->sa[signum].sa_handler = NULL;
		else
			current()->pcb->sa[signum] = *action;
	}

	return 0;
}


int do_kill(int pid, int sig)
{
	pcb_t *proc;
	unsigned long flags;

	flags = local_irq_save();

	if (sig < 0 || sig >= _NSIG) {
		printk("<kernel> %s: signal number not valid!\n", __func__);
		return -1;
	}

	proc = find_proc_by_pid(pid);
	if (proc == NULL) {
		printk("<kernel> %s: No process having the PID %d found!\n", __func__, pid);
		return -1;
	}

	/* Can send the signal only to living process. */

	if (proc->state != PROC_STATE_ZOMBIE) {
		/* Set the corresponding bit in the pcb signals bitmap */
		proc->sigset_map.sigmap[(sig-1) / (8*sizeof(long))] |= 1UL << (sig-1) % (8*sizeof(long));

		/* Depending on the scheduling policy, the signal handler will be processed soon. */
		raise_softirq(SCHEDULE_SOFTIRQ);
	}

	local_irq_restore(flags);

	return 0;
}
