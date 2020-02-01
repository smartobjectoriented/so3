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

#include <asm/memory.h>

/* The number of max threads must be aligned with the definition in so3.lds regarding the stack size. */
#define	THREAD_MAX		32
#define THREAD_NAME_LEN 	80

/* Per thread stack size. WARNING !! The size must be the same than the size declared in so3.lds. */
#define	THREAD_STACK_SIZE	(32 * 1024)

/* Default priority is set to 10 */
#define THREAD_PRIO_DEFAULT	10

#ifndef __ASSEMBLY__

#include <types.h>
#include <list.h>
#include <schedule.h>

typedef enum { THREAD_STATE_NEW, THREAD_STATE_READY, THREAD_STATE_RUNNING, THREAD_STATE_WAITING, THREAD_STATE_ZOMBIE } thread_state_t;
typedef unsigned int thread_t;

extern unsigned int __stack_top;

extern void thread_epilogue(void);

struct queue_thread;
typedef struct pcb pcb_t;
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

	/* Threaded function */
	int (*th_fn)(void *);
	void *th_arg;

	thread_t state;
	int stack_slotID; /* Thread kernel slot ID */

	/* Reference to the process, if any - typically NULL for kernel threads */
	pcb_t *pcb;
	int pcb_stack_slotID; /* This is the user space stack slot ID (for user threads) */

	int exit_status;

	struct list_head list;  /* List of threads belonging to a process */

	/* Join queue to handle threads waiting on it */
	struct list_head joinQueue;

	cpu_regs_t cpu_regs;
};
typedef struct tcb tcb_t;

typedef int(*th_fn_t)(void *);

void threads_init(void);

int do_thread_create(uint32_t *pthread_id, uint32_t attr_p, uint32_t thread_fn, uint32_t arg_p, uint32_t prio);
int do_thread_join(uint32_t pthread_id, int **value_p);
void do_thread_exit(int *exit_status);

tcb_t *kernel_thread(int (*start_routine) (void *), const char *name, void *arg, uint32_t prio);
tcb_t *user_thread(int (*start_routine) (void *), const char *name, void *arg, pcb_t *pcb, uint32_t prio);

int thread_join(tcb_t *tcb);
void thread_exit(int *exit_status);
void clean_thread(tcb_t *tcb);
void do_thread_yield(void);

int thread_idle(void *dummy);

uint32_t get_kernel_stack_top(uint32_t slotID);

extern void __switch_context(tcb_t *prev, tcb_t *next);
extern void __thread_prologue_kernel(void);
extern void __thread_prologue_user(void);
extern void __thread_prologue_user_pre_launch(void);

char *print_state(struct tcb *tcb);

#endif /* __ASSEMBLY__ */

#endif /* THREAD_H */
