/*
 * Copyright (C) 2014-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef THREAD_H
#define THREAD_H

#define THREAD_NAME_LEN 80

/* Default priority is set to 10 */
#define THREAD_PRIO_DEFAULT 10

#ifndef __ASSEMBLY__

#include <types.h>
#include <list.h>
#include <signal.h>
#include <syscall.h>

typedef enum {
	THREAD_STATE_NEW,
	THREAD_STATE_READY,
	THREAD_STATE_RUNNING,
	THREAD_STATE_WAITING,
	THREAD_STATE_ZOMBIE
} thread_state_t;
typedef unsigned int thread_t;

extern void thread_epilogue(void);

struct queue_thread;
typedef struct pcb pcb_t;

typedef void *(*th_fn_t)(void *);

/*
 * Task Control Block
 *
 * The structure is a self-contained list.
 */
struct tcb {
	int tid;
	char name[THREAD_NAME_LEN];

	/* Priority of this thread - the highest number the highest priority */
	/* Priority starts from 1 and as no limit at the moment. The prio 0 is used to indicate
	 * the default priority is used. */
	uint32_t prio;

	/* Timeout value to keep track of possible scheduling after a timeout. */
	int64_t timeout;

#ifdef CONFIG_SCHED_PRIO_DYN

	/* Used by the adaptative priority algorithm */
	uint32_t current_prio;

	/* Store the last time that the current_prio field has been incremented (ns). */
	u64 last_prio_inc_time;

#endif /* CONFIG_SCHED_PRIO_DYN */

	thread_t state;
	int stack_slotID; /* Thread kernel slot ID */

	/* Reference to the process, if any - typically NULL for kernel threads */
	pcb_t *pcb;

	int exit_status;
	int *clear_child_tid;

	/* Set when the thread is being force-terminated (e.g. the process is
	 * killed while this thread is blocked in the kernel). Blocking
	 * primitives check it after they resume and self-terminate cleanly
	 * once their own wait state has been unwound. See discard_tcb_in_pcb()
	 * and __sleep(). */
	bool killed;

#ifdef CONFIG_IPC_SIGNAL
	/* Set while the thread sits in an interruptible blocking primitive
	 * (see wait_for_completion_interruptible()). sys_do_kill() wakes such
	 * threads so they can observe the signal instead of sleeping until
	 * their completion happens to fire; the primitive then unwinds its own
	 * wait state and returns -EINTR. Only interruptible waiters are woken:
	 * the other primitives park a stack-allocated queue entry in a list and
	 * must not be resumed behind their back. */
	bool interruptible;

	/* Mask for thread's disabled signals */
	sigset_t sig_mask;
#endif

	struct list_head list; /* List of threads belonging to a process */

	/* Join queue to handle threads waiting on it */
	struct list_head joinQueue;

	cpu_regs_t cpu_regs;
};
typedef struct tcb tcb_t;

typedef struct {
	/* Clone flags to create/copy the current thread */
	uint64_t flags;
	const char *name;

	pcb_t *pcb;

	uint32_t prio;

	/* Userspace stack and TLS address */
	addr_t stack;
	addr_t tls;

	/* Pointer to userspace parent/child tid to use depending on flags */
	int *parent_tid;
	int *child_tid;

#ifdef CONFIG_IPC_SIGNAL
	/* Starting mask for thread's disabled signals, should be the same as parent */
	sigset_t sig_mask;
#endif

	/* Function to call for kernel thread or root user process */
	th_fn_t fn;

	/* Arguments passed to the kernel thread function. For userspace thread, this is ignored
	 * and arguments must put on top of the stack */
	void *fn_arg;
} clone_args_t;

void threads_init(void);

SYSCALL_DECLARE(exit, int exit_status);
SYSCALL_DECLARE(set_tid_address, int *tidptr);

tcb_t *kernel_thread(th_fn_t start_routine, const char *name, void *arg, uint32_t prio);
tcb_t *user_thread(clone_args_t *args);

int thread_join(tcb_t *tcb);
void thread_exit(int exit_status);
void clean_thread(tcb_t *tcb);
SYSCALL_DECLARE(sched_yield, void);

void *thread_idle(void *dummy);

addr_t get_kernel_stack_top(uint32_t slotID);

extern void __switch_context(tcb_t *prev, tcb_t *next);
extern void __thread_prologue_kernel(void);
extern void ret_from_fork(void);

char *print_state(struct tcb *tcb);

void *app_thread_main(void *args);

void arch_prepare_cpu_regs(tcb_t *tcb, clone_args_t *args);
void arch_restart_user_thread(tcb_t *tcb, th_fn_t fn_entry, addr_t stack);
addr_t arch_get_args_base(void);
addr_t arch_get_kernel_stack_frame(tcb_t *tcb);

#endif /* __ASSEMBLY__ */

#endif /* THREAD_H */
