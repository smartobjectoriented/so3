/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017 Xavier Ruppen <xavier.ruppen@heig-vd.ch>
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

#include <compiler.h>
#include <process.h>
#include <common.h>
#include <heap.h>
#include <errno.h>
#include <completion.h>
#include <schedule.h>
#include <memory.h>
#include <string.h>
#include <softirq.h>
#include <thread.h>

#include <asm/processor.h>

static unsigned int tid_next = 0;

char *state_str[] = { "NEW", "READY", "RUNNING", "WAITING", "ZOMBIE" };

/*
 * Display a convenient string to print the state of a thread.
 */
char *print_state(struct tcb *tcb)
{
	return state_str[tcb->state];
}

/*
 * Find a thread (tcb_t) from its tid. The thread belongs to the process @pcb.
 *
 * Return NULL if no process as been found.
 */
tcb_t *find_thread_by_tid(pcb_t *pcb, uint32_t tid)
{
	tcb_t *tcb;
	struct list_head *pos;

	list_for_each(pos, &pcb->threads) {
		tcb = list_entry(pos, tcb_t, list);

		if (tcb->tid == tid)
			return tcb;
	}

	/* Not found */
	return NULL;
}

/*
 * Remove a tcb from the list belonging to the @pcb
 * If @tcb is the process main_thread, we perform removal on all tcb (except the main thread of course,
 * which will be cleaned by a subsequent call to waitpid().
 */
void remove_tcb_from_pcb(tcb_t *tcb)
{
	tcb_t *cur;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, &tcb->pcb->threads) {
		cur = list_entry(pos, tcb_t, list);

		if ((tcb == tcb->pcb->main_thread) || (tcb == cur)) {
			list_del(pos);
			clean_thread(cur);

			if (tcb != tcb->pcb->main_thread)
				return;
		}
	}
}

/*
 * Discarding all tcb spawned in a process (except the main_thread of course)
 */
void discard_tcb_in_pcb(pcb_t *pcb)
{
	tcb_t *cur;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, &pcb->threads) {
		cur = list_entry(pos, tcb_t, list);

		/* Check if the tcb is in a ready thread ? */
		if (cur->state == THREAD_STATE_READY)
			remove_ready(cur);
		else if (cur->state == THREAD_STATE_WAITING)
			BUG(); /* Not handled yet... */

		list_del(pos);
		clean_thread(cur);
	}
}

/*
 * Returns the number of running (active) threads in a process
 */
uint32_t active_threads(pcb_t *pcb)
{
	tcb_t *cur;
	struct list_head *pos;
	uint32_t threads = 0;

	list_for_each(pos, &pcb->threads) {
		cur = list_entry(pos, tcb_t, list);

		if (cur->state != THREAD_STATE_ZOMBIE) {
			threads++;
		}
	}

	return threads;
}

/*
 * Thread stack management
 */
bool kernel_stack_slot[CONFIG_MAX_THREADS];

/*
 * Get the kernel stack (top), full descending - The kernel stack is divided into small stack areas
 * used for individual kernel *and* user threads (in SVC mode).
 *
 * The first stack area is the initial system stack.
 * The first thread stack slot ID #0 starts right after the initial system stack.
 */
addr_t get_kernel_stack_top(uint32_t slotID)
{
	addr_t stack_top;

	/* Set the bottom to the real CPU stack area */
	stack_top = (addr_t) ((void *) &__stack_bottom) +
		    smp_processor_id() * (CONFIG_SYS_STACK_SIZE_KB + CONFIG_MAX_THREADS * CONFIG_THREAD_STACK_SIZE_KB) * SZ_1K;

	/* Skip the system stack */
	stack_top += CONFIG_SYS_STACK_SIZE_KB * SZ_1K;

	/* Move to the top of stack of corresponding slot */
	stack_top += (slotID + 1) * CONFIG_THREAD_STACK_SIZE_KB * SZ_1K;

	return stack_top;
}

/*
 * Reserve a stack slot.
 * This function returns the stack base address.
 *
 * The general stack base address is retrieved from __svc_start__.
 * No need to reserve the first stack sloFt.
 */
int get_kernel_stack_slot(void)
{
	unsigned int i;

	for (i = 0; i < CONFIG_MAX_THREADS; i++) {
		if (!kernel_stack_slot[i]) {
			kernel_stack_slot[i] = true;

			return i;
		}
	}

	/* No free stack slot */
	return -1;
}

/* Free a kernel stack slot. */
void free_kernel_stack_slot(int slotID)
{
	kernel_stack_slot[slotID] = false;
}

#warning free_queue_thread() unused at the moment...
void free_queue_thread(struct list_head *aList)
{
	queue_thread_t *cur;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, aList) {
		cur = list_entry(pos, queue_thread_t, list);

		list_del(pos);
		free(cur);
	}
}

/*
 * Thread exit routine
 */
void thread_exit(int exit_status)
{
	queue_thread_t *cur;
	struct list_head *pos, *q;
	pcb_t *pcb;

	local_irq_disable();

	ASSERT(current()->state == THREAD_STATE_RUNNING);

	current()->exit_status = exit_status;

	/*
	 * According to the thread which is calling thread_exit(), the behaviour may differ.
	 * Typically, if it is the main thread of the process, we have to wait until all
	 * running threads (belonging to the process) are completed, and we pursue with
	 * sys_do_exit().
	 *
	 * - If pcb->state == PROC_STATE_ZOMBIE, it means we are called from sys_do_exit()
	 */
	pcb = current()->pcb;

	if (pcb && (current() == pcb->main_thread) && (pcb->state != PROC_STATE_ZOMBIE)) {
		while (active_threads(pcb) > 0) {
			local_irq_enable();
			wait_for_completion(&pcb->threads_active);
			local_irq_disable();
		}

		/* Make sure all tcb have disappeared */
		remove_tcb_from_pcb(current());

#ifdef CONFIG_PROC_ENV
		sys_do_exit_group(0);
#endif

	} else {
		if (pcb && (current() == pcb->main_thread))
			/* Discard all threaded spawned within this process */
			discard_tcb_in_pcb(pcb);

		/* If a kernel thread is finishing, or a thread in a process which performed a call to exec(),
		 * and no other threads are waiting on it, we can disappear from the system. */

		if (!list_empty(&current()->joinQueue)) {
			/* Wake up possible waiting threads */
			list_for_each_safe(pos, q, &current()->joinQueue) {
				cur = list_entry(pos, queue_thread_t, list);

				ready(cur->tcb);

				/* The entry will be freed by a joining thread, i.e. the last one to be woken up. */
			}

			/* Special case for handling the treads_active completion. If we are the last thread (apart main), we
			 * have to release the completion.
			 */

			if (pcb && (active_threads(pcb) == 1)) /* We are the last one, and we will be put in zombie state... */
				complete(&pcb->threads_active);
		}

		/* If clear child tid is set, inform waiting thread */
		if (pcb && current() != pcb->main_thread) {
			if (current()->clear_child_tid) {
				*(current()->clear_child_tid) = 0;
				sys_do_futex(current()->clear_child_tid, FUTEX_WAKE, 1, NULL, NULL, 0);
			}
		}

		/* Check if this is a kernel thread OR a main thread before a image replacement (during exec()).
		 * In this case, we do not leave the thread in zombie since we assume that no other threads are joining on it.
		 * (This is currently a limitation: kernel threads can not join other threads).
		 */
#warning Kernel threads cannot join other threads!

		if (current()->pcb != NULL)
			zombie();
		else {
			clean_thread(current());
			set_current(NULL);
			schedule();
		}

		/* Should never reach this point... */
		BUG();
	}
}

/*
 * The thread prologue starts its execution in USER mode.
 */
void thread_prologue(void (*th_fn)(void *arg), void *arg)
{
	/* Call the function of the user space */
	th_fn(arg);

	/*
	 * Following the execution of the thread, either the user thread
	 * is calling pthread_exit() explicitly, or the pthread core function
 	 * will do it.
 	 * If we reach this point, it means that a kernel thread has finished its execution.
	 */

	thread_exit(0);
}

/*
 * Thread idle
 */
void *thread_idle(void *dummy)
{
	/* Endless loop */
	while (true) {
		/* For the moment nothing... The call to schedule() is ensured right after an IRQ processing. */

#ifdef CONFIG_RTOS
		/* In RTOS, we do not have a periodic timer which give the hand to schedule() */
		schedule();

		local_irq_disable();
		do_softirq();
		local_irq_enable();

#endif /* CONFIG_RTOS */

		cpu_standby();
	}

	return NULL;
}

/*
 * Yield to another thread, i.e. simply invoke a call to schedule()
 */
SYSCALL_DEFINE0(thread_yield)
{
	schedule();
	return 0;
}

void set_thread_registers(tcb_t *thread, cpu_regs_t *regs)
{
	memcpy(&thread->cpu_regs, regs, sizeof(cpu_regs_t));
}

/**
 * Thread creation routine
 *
 * @param args Aguments to create the thread
 *
 * @return TCB of the newly created thread.
 */
tcb_t *thread_create(clone_args_t *args)
{
	tcb_t *tcb;
	unsigned long flags;

	BUG_ON(boot_stage < BOOT_STAGE_SCHED);

	flags = local_irq_save();

	tcb = (tcb_t *) calloc(1, sizeof(tcb_t));

	if (tcb == NULL) {
		printk("%s: failed to alloc memory.\n", __func__);
		kernel_panic();
	}

	tcb->tid = tid_next++;

	/* We append the tid to the thread name */
	snprintf(tcb->name, THREAD_NAME_LEN, "%s_%d", args->name, tcb->tid);

	if (args->prio)
		tcb->prio = args->prio;
	else
		tcb->prio = THREAD_PRIO_DEFAULT;

	tcb->state = THREAD_STATE_NEW;
	tcb->pcb = args->pcb;

#ifdef CONFIG_IPC_SIGNAL
	tcb->sig_mask = args->sig_mask;
#endif

	/* Init the thread kernel stack (svc mode) for both kernel and user thread. */
	tcb->stack_slotID = get_kernel_stack_slot();

	if (tcb->stack_slotID < 0) {
		printk("Stack overflow: no available stack for a new thread\n");
		kernel_panic();
	}

	/* Prepare registers for future restore in switch_context() */
	arch_prepare_cpu_regs(tcb, args);

	/* Initialize the join queue associated to this thread */
	INIT_LIST_HEAD(&tcb->joinQueue);

	/* We do not want to have the idle thread as a "ready" thread */
	if (args->fn != thread_idle)
		ready(tcb);

	if (args->flags & CLONE_PARENT_SETTID) {
		/* Return the TID of the child thread. The child will not execute
		 * this code, since it jumps to the ret_from_fork in context.S
		 */
		*args->parent_tid = tcb->tid;
	}

	if (args->flags & CLONE_CHILD_CLEARTID)
		tcb->clear_child_tid = args->child_tid;

	local_irq_restore(flags);

	return tcb;
}

/**
 * Create a new kernel thread.
 *
 * @param start_routine Kernel function to call in the thread.
 * @param name Name of the thread.
 * @param arg Arguments passed to the function called by the thread.
 * @param prio The thread scheduling priority.
 *
 * @return TCB of the newly created thread.
 */
tcb_t *kernel_thread(th_fn_t start_routine, const char *name, void *arg, uint32_t prio)
{
	clone_args_t args = {
		.name = name,
		.fn = start_routine,
		.fn_arg = arg,
		.prio = prio,
	};

	return thread_create(&args);
}

/**
 * Create a new user thread based on given arguments.
 *
 * @param args Aguments to create the user thread
 *
 * @return TCB of the newly created thread.
 */
tcb_t *user_thread(clone_args_t *args)
{
	tcb_t *tcb;

	args->prio = (args->flags & CLONE_THREAD) ? args->pcb->main_thread->prio : 0;

	tcb = thread_create(args);

	if (args->flags & CLONE_THREAD) {
		/* Insert the newly thread to the thread list of this process */
		list_add_tail(&tcb->list, &args->pcb->threads);
	} else {
		BUG_ON(args->pcb->main_thread != NULL);
		args->pcb->main_thread = tcb;
	}

	return tcb;
}

/*
 * @clean_thread is called when a final join on the (last) thread is performed, or an exec() is performed
 * so that the binary image is fully replaced.
 */
void clean_thread(tcb_t *tcb)
{
	/* Definitively remove the thread */
	free_kernel_stack_slot(tcb->stack_slotID);

	ASSERT(list_empty(&tcb->joinQueue));

	/* Remove the thread from the zombie list */
	if (tcb->state == THREAD_STATE_ZOMBIE)
		remove_zombie(tcb);

	/* Finally, free the tcb struct */
	free(tcb);
}

/*
 * Join a thread.
 * Perform synchronization and remove the tcb of the joined thread.
 *
 * The target thread which we are trying to join will be definitively removed
 * from the system when no other threads are joining it too.
 */
int thread_join(tcb_t *tcb)
{
	queue_thread_t *cur;
	int exit_status;
	tcb_t *_tcb;
	struct list_head *pos, *q;
	unsigned long flags;
	bool is_main_thread;
	pcb_t *__child_pcb;

	flags = local_irq_save();

	is_main_thread = ((tcb->pcb != NULL) && (tcb == tcb->pcb->main_thread));

	/* Check if the thread already finished */
	if (tcb->state != THREAD_STATE_ZOMBIE) {
		/* Register us in the join queue attached to the target thread */

		cur = (queue_thread_t *) malloc(sizeof(queue_thread_t));
		if (cur == NULL) {
			printk("%s: cannot allocate memory to register in join queue.\n", __func__);
			BUG();
		}

		cur->tcb = current();

		/* Append ourself in the queue */
		list_add_tail(&cur->list, &tcb->joinQueue);

		if (tcb->pcb != NULL)
			__child_pcb = tcb->pcb;

		/* Waiting for the target thread; we suspend ourself... */
		waiting();

		/* Check if the main_thread has be replaced by a new one during an exec() operation
		 * performed in the child.
		 */
		if (is_main_thread)
			tcb = __child_pcb->main_thread; /* Proceed with substitution; the other thread does not exist anymore. */

		/*
		 * At this point, we are awake; the target thread is over in *zombie* state.
		 * So, we leave the join queue belonging to this thread.
		 */

		list_for_each_safe(pos, q, &tcb->joinQueue) {
			cur = list_entry(pos, queue_thread_t, list);
			_tcb = cur->tcb;

			if (_tcb == current()) {
				list_del(pos);
				free(cur);
				break;
			}
		}
	}

	/* The joined thread *must* be in zombie */
	ASSERT(tcb->state == THREAD_STATE_ZOMBIE);

	if (is_main_thread)
		exit_status = tcb->pcb->exit_status;
	else
		exit_status = tcb->exit_status;

	/*
	 * Now, if we are the last which is woken up, we can proceed with the tcb removal.
	 * If the join is done on a main_thread, it means the waiting parent is doing the join, and
	 * we can then clean the tcb (remember that the main_thread does not appear in the list
	 * of threads of the PCB; only created threads are in it.
	 */

	if (list_empty(&tcb->joinQueue)) {
		if (is_main_thread) {
			clean_thread(tcb);
			tcb->pcb->main_thread = NULL;
		} else
			/* Remove the tcb from the list of threads owned by this process */
			remove_tcb_from_pcb(tcb);
	}
	local_irq_restore(flags);

	return exit_status;
}

/*
 * do_thread_exit() is called when pthread_exit() is executed.
 */
SYSCALL_DEFINE1(exit, int, exit_status)
{
	/* Unallocate the user space stack slot if it is not the main thread */
	thread_exit(exit_status);

	return 0;
}

SYSCALL_DEFINE1(set_tid_address, int *, tidptr)
{
	current()->clear_child_tid = tidptr;
	return current()->tid;
}

void threads_init(void)
{
	int i;

	for (i = 0; i < CONFIG_MAX_THREADS; i++)
		kernel_stack_slot[i] = false; /* Unallocated stack */
}
