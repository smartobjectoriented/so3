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

char *state_str[] = {
		"NEW",
		"READY",
		"RUNNING",
		"WAITING",
		"ZOMBIE"
};

/*
 * Display a convenient string to print the state of a thread.
 */
char *print_state(struct tcb *tcb) {
	return state_str[tcb->state];
}


/*
 * Find a thread (tcb_t) from its tid. The thread belongs to the process @pcb.
 *
 * Return NULL if no process as been found.
 */
tcb_t *find_thread_by_tid(pcb_t *pcb, uint32_t tid) {
	tcb_t *tcb;
	struct list_head *pos;

	list_for_each(pos, &pcb->threads)
	{
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
void remove_tcb_from_pcb(tcb_t *tcb) {
	tcb_t *cur;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, &tcb->pcb->threads)
	{
		cur = list_entry(pos, tcb_t, list);

		if ((tcb == tcb->pcb->main_thread) || (tcb == cur)) {
			list_del(pos);
			clean_thread(cur);

			if (tcb != tcb->pcb->main_thread)
				return ;
		}
	}
}

/*
 * Discarding all tcb spawned in a process (except the main_thread of course)
 */
void discard_tcb_in_pcb(pcb_t *pcb) {
	tcb_t *cur;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, &pcb->threads)
	{
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
uint32_t active_threads(pcb_t *pcb) {
	tcb_t *cur;
	struct list_head *pos;
	uint32_t threads = 0;

	list_for_each(pos, &pcb->threads)
	{
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
bool kernel_stack_slot[THREAD_MAX];

/*
 * Get the kernel stack (top), full descending - The kernel stack is divided into small stack areas
 * used for individual kernel *and* user threads (in SVC mode).
 *
 * The first stack area is the initial system stack and remains preserved so far.
 * The first thread stack slot ID #0 starts right under this area.
 */
addr_t get_kernel_stack_top(uint32_t slotID) {
	return (addr_t) ((void *) &__stack_top - THREAD_STACK_SIZE - slotID*THREAD_STACK_SIZE);
}

/*
 * Reserve a stack slot.
 * This function returns the stack base address.
 *
 * The general stack base address is retrieved from __svc_start__.
 * No need to reserve the first stack slot.
 */
int get_kernel_stack_slot(void)
{
	unsigned int i;

	for (i = 0; i < THREAD_MAX; i++) {
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

/* Process thread stack management */

/* Get the kernel stack (top), full descending */
addr_t get_user_stack_top(pcb_t *pcb, uint32_t slotID) {
	return (addr_t) ((void *) pcb->stack_top - slotID*THREAD_STACK_SIZE);
}

/*
 * Reserve a (user space) stack slot dedicated to a user thread.
 */
int get_user_stack_slot(pcb_t *pcb)
{
	unsigned int i;

	for (i = 0; i < PROC_THREAD_MAX; i++) {
		if (!pcb->stack_slotID[i]) {
			pcb->stack_slotID[i] = true;

			return i;
		}
	}

	/* No free stack slot */
	return -1;
}

/*
 * Release the user space stack slot.
 */
void free_user_stack_slot(pcb_t *pcb, int slotID)
{
	pcb->stack_slotID[slotID] = false;
}


#warning free_queue_thread() unused at the moment...
void free_queue_thread(struct list_head *aList)
{
	queue_thread_t *cur;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, aList)
	{
		cur = list_entry(pos, queue_thread_t, list);

		list_del(pos);
		free(cur);
	}
}

/*
 * Thread exit routine
 */
void thread_exit(int *exit_status)
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
	 * do_exit().
	 *
	 * - If pcb->state == PROC_STATE_ZOMBIE, it means we are called from do_exit()
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
		do_exit(0);
#endif

	} else {

		if (pcb && (current() == pcb->main_thread))
			/* Discard all threaded spawned within this process */
			discard_tcb_in_pcb(pcb);

		/* If a kernel thread is finishing, or a thread in a process which performed a call to exec(),
		 * and no other threads are waiting on it, we can disappear from the system. */

		if (!list_empty(&current()->joinQueue)) {

			/* Wake up possible waiting threads */
			list_for_each_safe(pos, q, &current()->joinQueue)
			{
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
void thread_prologue(void(*th_fn)(void *arg), void *arg)
{
	/* Call the function of the user space */
	th_fn(arg);

	/*
	 * Following the execution of the thread, either the user thread
	 * is calling pthread_exit() explicitly, or the pthread core function
 	 * will do it.
 	 * If we reach this point, it means that a kernel thread has finished its execution.
	 */

	thread_exit(NULL);

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
void do_thread_yield(void) {
	schedule();
}

void set_thread_registers(tcb_t *thread, cpu_regs_t *regs)
{
	memcpy(&thread->cpu_regs, regs, sizeof(cpu_regs_t));
}

/*
 * Thread creation routine
 *
 * @start_routine: function to be threaded; if NULL, it means we are along a fork() processing.
 * @name: name of the thread. Warning ! The caller must foresee a concatenation of the tid to the string.
 * @arg: pointer to possible args
 * @pcb: NULL means it is a pure kernel thread, otherwise is is a user thread.
 * @prio: 0 means the default priority, otherwise set to the thread to the corresponding priority
 */
tcb_t *thread_create(th_fn_t start_routine, const char *name, void *arg, pcb_t *pcb, uint32_t prio)
{
	tcb_t *tcb;
	unsigned long flags;

	BUG_ON(boot_stage < BOOT_STAGE_SCHED);

	flags = local_irq_save();

	tcb = (tcb_t *) malloc(sizeof(tcb_t));

	if (tcb == NULL) {
		printk("%s: failed to alloc memory.\n", __func__);
		kernel_panic();
	}

	tcb->tid = tid_next++;
	
	/* We append the tid to the thread name */
	snprintf(tcb->name, THREAD_NAME_LEN, "%s_%d", name, tcb->tid);

	tcb->th_fn = start_routine;
	tcb->th_arg = arg;

	if (prio)
		tcb->prio = prio;
	else
		tcb->prio = THREAD_PRIO_DEFAULT;

	tcb->state = THREAD_STATE_NEW;
	tcb->pcb = pcb;

	/* Init the thread kernel stack (svc mode) for both kernel and user thread. */
	tcb->stack_slotID = get_kernel_stack_slot();

	if (tcb->stack_slotID < 0) {
		printk("Stack overflow: no available stack for a new thread\n");
		kernel_panic();
	}

	/* Prepare the user stack if any related PCB */
	if (tcb->pcb) {
		/* Prepare the user stack */
		tcb->pcb_stack_slotID = get_user_stack_slot(tcb->pcb);
		if (tcb->pcb_stack_slotID < 0) {
			printk("No available user stack for a new thread\n");
			kernel_panic();
		}
	}

	/* Prepare registers for future restore in switch_context() */
	arch_prepare_cpu_regs(tcb);

	/* Quite common registers on various architectures */
	tcb->cpu_regs.sp = get_kernel_stack_top(tcb->stack_slotID);

	if (tcb->pcb)
		tcb->cpu_regs.lr = (unsigned long) __thread_prologue_user;
	 else
		tcb->cpu_regs.lr = (unsigned long) __thread_prologue_kernel;

	/* Initialize the join queue associated to this thread */
	INIT_LIST_HEAD(&tcb->joinQueue);

	/* We do not want to have the idle thread as a "ready" thread */
	if (start_routine != thread_idle)
		ready(tcb);

	local_irq_restore(flags);

	return tcb;
}

tcb_t *kernel_thread(th_fn_t start_routine, const char *name, void *arg, uint32_t prio)
{
	return thread_create(start_routine, name, arg, NULL, prio);
}

/**
 * Should not be called directly.
 * Called by create_process() or create_child_thread() instead.
 *
 * @param start_routine	Address ot the thread routine
 * @param name		Name of the thread
 * @param arg		Argument of the thread
 * @param pcb		PCB which the thread belongs to
 * @return		Address of the corresponding TCB
 */
tcb_t *user_thread(th_fn_t start_routine, const char *name, void *arg, pcb_t *pcb)
{
	return thread_create(start_routine, name, arg, pcb, (pcb->main_thread ? pcb->main_thread->prio : 0));
}

/*
 * @clean_thread is called when a final join on the (last) thread is performed, or an exec() is performed
 * so that the binary image is fully replaced.
 */
void clean_thread(tcb_t *tcb) {

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
int *thread_join(tcb_t *tcb)
{
	queue_thread_t *cur;
	int *exit_status;
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

		list_for_each_safe(pos, q, &tcb->joinQueue)
		{
			cur = list_entry(pos, queue_thread_t, list);
			_tcb = cur->tcb;

			if (_tcb == current()) {

				list_del(pos);
				free(cur);
				break;
			}
		}
	}

	/* Check if the child is a tracee (and therefore we have a tracer on it) */
	if ((tcb != NULL) && (tcb->pcb->ptrace_pending_req != PTRACE_NO_REQUEST)) {
		
		exit_status = NULL;

	} else {
		
		/* The joined thread *must* be in zombie */
		ASSERT(tcb->state == THREAD_STATE_ZOMBIE);

		if (is_main_thread)
			exit_status = (void *) ((unsigned long) tcb->pcb->exit_status);
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
	}
	local_irq_restore(flags);

	return exit_status;
}

/*
 * do_thread_create is the syscall implementation behind the pthread_create() called in the libc.
 *
 * @pthread_p refers to the address of the resulting pthread_t id
 * @attr_p refers to the address of pthread attributes
 * @thread_fn refers to the threaded function in the user space
 * @arg_p refers to the arguments passed to the threaded function
 *
 * The function returns 0 if successful.
 */

int do_thread_create(uint32_t *pthread_id, addr_t attr_p, addr_t thread_fn, addr_t arg_p) {

	unsigned long flags;
	tcb_t *tcb;
	char *name;

	flags = local_irq_save();
 
	/* Create a child thread for the running process */

	/* Temporary name for this thread */
	name = (char *) malloc(THREAD_NAME_LEN);
	if (!name) {
		printk("%s: heap overflow...\n", __func__);
		kernel_panic();
	}
	snprintf(name, THREAD_NAME_LEN, "thread_p%d", current()->pcb->pid);

	tcb = user_thread((th_fn_t) thread_fn, name, (void *) arg_p, current()->pcb);

	/* The name has been copied in thread creation */
	free(name);

	/* Insert the newly thread to the thread list of this process */
	list_add_tail(&tcb->list, &current()->pcb->threads);

	*pthread_id = tcb->tid;

	local_irq_restore(flags);

	return 0;
}

/*
 * Join an existing thread
 */
int do_thread_join(uint32_t pthread_id, int **value_p) {
	tcb_t *tcb;
	int *ret;
	unsigned long flags;

	flags = local_irq_save();

	tcb = find_thread_by_tid(current()->pcb, pthread_id);

	if (tcb == NULL) {
		set_errno(EINVAL);
		return -1;
	}

	ret = thread_join(tcb);

	if (value_p != NULL)
		*value_p = ret;

	local_irq_restore(flags);

	return 0;
}

/*
 * do_thread_exit() is called when pthread_exit() is executed.
 */
void do_thread_exit(int *exit_status) {

	/* Unallocate the user space stack slot if it is not the main thread */
	if (current() != current()->pcb->main_thread)
		free_user_stack_slot(current()->pcb, current()->pcb_stack_slotID);

	thread_exit(exit_status);
}

void threads_init(void)
{
	int i;

	for (i = 0; i < THREAD_MAX; i++)
		kernel_stack_slot[i] = false; /* Unallocated stack */
}
