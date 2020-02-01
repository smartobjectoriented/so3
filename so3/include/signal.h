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

#ifndef SIGNAL_H
#define SIGNAL_H

#include <types.h>
#include <thread.h>

#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGIOT    SIGABRT
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGSTKFLT 16
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGURG    23
#define SIGXCPU   24
#define SIGXFSZ   25
#define SIGVTALRM 26
#define SIGPROF   27
#define SIGWINCH  28
#define SIGIO     29
#define SIGPOLL   29
#define SIGPWR    30
#define SIGSYS    31
#define SIGUNUSED SIGSYS

#define _NSIG 65

#define SIGMAP_CELLSIZE (sizeof(int))

typedef void (*__sighandler_t)(int);

typedef struct {
	uint32_t sigmap[_NSIG/(8*sizeof(uint32_t))];
} sigset_t;

/* Fake signal functions.  */

#define	SIG_ERR	 ((__sighandler_t) -1)	/* Error return.  */
#define	SIG_DFL	 ((__sighandler_t)  0)	/* Default action.  */
#define	SIG_IGN	 ((__sighandler_t)  1)	/* Ignore signal.  */

/*
 * This structure must match the k_sigaction structure used in the libc.
 * For now, sa_mask and sa_flags are not used.
 */
typedef struct sigaction {
		__sighandler_t	sa_handler;
		unsigned long	sa_flags;
		void		(*sa_restorer)(int, void *);
		sigset_t	sa_mask;
} sigaction_t;

/*
 * __sigaction_t is used by the assembly glue to retrieve the signal number and the sigaction_t info.
 */
typedef struct __sigaction {
	int 		signum;
	sigaction_t 	*sa;
} __sigaction_t;

int do_sigaction(int signum, const sigaction_t *action, sigaction_t *old_action);
int do_kill(int pid, int sig);
void do_sigreturn(void);

#endif /* SIGNAL_H */
