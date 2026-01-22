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

#include <signal.h>
#include <schedule.h>
#include <process.h>
#include <thread.h>
#include <types.h>
#include <softirq.h>
#include <log.h>

#include <asm/processor.h>

SYSCALL_DEFINE0(sigreturn)
{
	/* Nothing at the moment ... */

	/* We should actually verify if sigreturn is called by the libc during
	 * a signal handler processing, or whether it is called by the user itself.
	 * The latter case should lead to returning -1. Will be implemented later on.
	 */

	/*
	 * Cleaning the stack frame used for signal handling will be carried out
	 * during along the return path to the user space.
	 */

	return 0;
}

/**
 * rt_sigreturn isn't different from sigreturn for SO3 use case.
 */
SYSCALL_DEFINE0(rt_sigreturn)
{
	return sys_do_sigreturn();
}

/**
 * Checks if signals were set and executes their handlers if so.
 */
__sigaction_t *sig_check(void)
{
	size_t i;
	size_t sig_nr;
	sigset_t *signal;
	sigset_t *blocked;
	unsigned long pending;
	unsigned long bit_pos;

	if (!current() || (current()->pcb == NULL))
		return NULL;

	signal = &current()->pcb->sigset_map;
	blocked = &current()->sig_mask;

	for (i = 0; i < _NSIG_NB_ENTRY; i++) {
		/* Find if any unmasked signal is pending */
		pending = signal->sigmap[i] & ~blocked->sigmap[i];

		while (pending != 0) {
			/* Get number of the next pending signal */
			bit_pos = find_first_bit(&pending, _NSIG_BPE);
			sig_nr = bit_pos + (i * _NSIG_BPE) + 1;

			/* Reset pending state of the handled signal. */
			signal->sigmap[i] &= ~(1UL << bit_pos);
			pending &= ~(1UL << bit_pos);

			/* Check if a handler was specified for this signal */
			if (current()->pcb->sa[sig_nr].sa_handler == SIG_DFL) {
				/* Default handler to terminate an application */
				if ((sig_nr == SIGINT) || (sig_nr == SIGTERM) || (sig_nr == SIGKILL))
					sys_do_exit_group(0);

				continue;
			}

			if (current()->pcb->sa[sig_nr].sa_handler != SIG_IGN) {
				/* User set handler, let return in it */
				current()->pcb->__sa[sig_nr].sa = &current()->pcb->sa[sig_nr];
				current()->pcb->__sa[sig_nr].signum = sig_nr;

				return &current()->pcb->__sa[sig_nr];
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

SYSCALL_DEFINE4(rt_sigaction, int, signum, const sigaction_t *, action, sigaction_t *, old_action, size_t, sigsize)
{
	if (sigsize != sizeof(sigset_t)) {
		LOG_WARNING("Invalid sigset size\n");
		return -EINVAL;
	}

	if (signum < 0 || signum >= _NSIG) {
		LOG_ERROR("signum not valid!\n");
		return -EINVAL;
	}

	if ((signum == SIGKILL) || (signum == SIGTERM)) {
		LOG_ERROR("SIGKILL and SIGTERM can't be modified!\n");
		return -EINVAL;
	}

	if (old_action != NULL)
		*old_action = current()->pcb->sa[signum];

	/* Copy action into the process sigaction_t array */
	if (action)
		current()->pcb->sa[signum] = *action;

	return 0;
}

SYSCALL_DEFINE4(rt_sigprocmask, int, how, const sigset_t *, set, sigset_t *, old, size_t, sigsize)
{
	size_t i;

	if (sigsize != sizeof(sigset_t)) {
		LOG_WARNING("Invalid sigset size\n");
		return -EINVAL;
	}

	if (old != NULL)
		*old = current()->sig_mask;

	if (set != NULL) {
		switch (how) {
		case SIG_BLOCK:
			for (i = 0; i < _NSIG_NB_ENTRY; i++) {
				current()->sig_mask.sigmap[i] |= set->sigmap[i];
			}
			break;
		case SIG_UNBLOCK:
			for (i = 0; i < _NSIG_NB_ENTRY; i++) {
				current()->sig_mask.sigmap[i] &= ~set->sigmap[i];
			}
			break;
		case SIG_SETMASK:
			current()->sig_mask = *set;
			break;

		default:
			return -EINVAL;
		}

		/* Ensure SIGKILL and SIGTERM are still active */
		current()->sig_mask.sigmap[(SIGKILL - 1) / _NSIG_BPE] &= ~(1UL << ((SIGKILL - 1) % _NSIG_BPE));
		current()->sig_mask.sigmap[(SIGTERM - 1) / _NSIG_BPE] &= ~(1UL << ((SIGTERM - 1) % _NSIG_BPE));
	}

	return 0;
}

SYSCALL_DEFINE2(kill, int, pid, int, sig)
{
	pcb_t *proc;
	unsigned long flags;

	flags = local_irq_save();

	if (sig < 0 || sig >= _NSIG) {
		LOG_ERROR("<kernel> %s: signal number not valid!\n", __func__);
		return -1;
	}

	proc = find_proc_by_pid(pid);
	if (proc == NULL) {
		LOG_ERROR("<kernel> %s: No process having the PID %d found!\n", __func__, pid);
		return -1;
	}

	/* Can send the signal only to living process. */

	if (proc->state != PROC_STATE_ZOMBIE) {
		/* Set the corresponding bit in the pcb signals bitmap */
		proc->sigset_map.sigmap[(sig - 1) / _NSIG_BPE] |= 1UL << (sig - 1) % _NSIG_BPE;

		/* Depending on the scheduling policy, the signal handler will be processed soon. */
		raise_softirq(SCHEDULE_SOFTIRQ);
	}

	local_irq_restore(flags);

	return 0;
}
