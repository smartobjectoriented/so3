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

#if 0
#define DEBUG
#endif

#include <compiler.h>
#include <schedule.h>
#include <process.h>
#include <thread.h>
#include <heap.h>
#include <softirq.h>
#include <mutex.h>
#include <timer.h>

#include <device/irq.h>

#include <asm/mmu.h>

static struct list_head readyThreads;
static struct list_head zombieThreads;

/* Global list of process */
struct list_head proc_list;
struct tcb *tcb_idle = NULL;

tcb_t *current_thread;
static sched_policy_t sched_policy;

static spinlock_t schedule_lock;

volatile u64 jiffies = 0ull;
volatile u64 jiffies_ref = 0ull;

static volatile bool __sched_preempt = false;

static timer_t schedule_timer;

/*
 * The following code (normally disabled) is used for debugging purposes...
 */
#if 0
inline bool check_consistency_ready(void) {
	queue_thread_t *cur;
	tcb_t *_tcb;
	struct list_head *pos;

	list_for_each(pos, &readyThreads)
	{
		cur = list_entry(pos, queue_thread_t, list);
		_tcb = cur->tcb;
		if (_tcb->state != THREAD_STATE_READY) {
			printk("### ERROR %d on tid %d with state: %s ###\n", __LINE__, _tcb->tid, print_state(_tcb));
			return false;
		}
	}
	return true;
}
#endif /* 0 */

void preempt_disable(void) {
	unsigned long flags;

	flags = spin_lock_irqsave(&schedule_lock);
	__sched_preempt = false;
	spin_unlock_irqrestore(&schedule_lock, flags);
}

void preempt_enable(void) {
	unsigned long flags;

	flags = spin_lock_irqsave(&schedule_lock);
	__sched_preempt = true;
	spin_unlock_irqrestore(&schedule_lock, flags);
}

/*
 * Insert a new thread in the ready list.
 */
void ready(tcb_t *tcb) {
	unsigned long flags;
	queue_thread_t *cur;
	bool already_locked;

	/* We check if we are in a call path where the lock was already acquired.
	 * These are rare cases where consistency check is performed before calling
	 * ready() like waking up threads (see wake_up()).
	 */
	already_locked = (spin_is_locked(&schedule_lock) ? true : false);

	if (!already_locked)
		flags = spin_lock_irqsave(&schedule_lock);

	tcb->state = THREAD_STATE_READY;

	cur = (queue_thread_t *) malloc(sizeof(queue_thread_t));
	BUG_ON(cur == NULL);
	
#ifdef CONFIG_SCHED_PRIO_DYN
	
	/* Reset priority increment timer */
	tcb->last_prio_inc_time = NOW();

	if (tcb->current_prio > tcb->prio)
		tcb->current_prio--;

#endif /* CONFIG_SCHED_PRIO_DYN */

	cur->tcb = tcb;

	/* Insert the thread at the end of the list */
	list_add_tail(&cur->list, &readyThreads);

	if (!already_locked)
		spin_unlock_irqrestore(&schedule_lock, flags);
}

/*
 * Put a thread in the waiting state and invoke the scheduler.
 * It is assumed that the caller manages the waiting queue.
 */
void waiting(void) {
	unsigned long flags;

	flags = local_irq_save();

	ASSERT(current()->state == THREAD_STATE_RUNNING);

	current()->state = THREAD_STATE_WAITING;

	schedule();

	local_irq_restore(flags);
}

/*
 * Put a thread into the zombie queue.
 */
void zombie(void) {
	queue_thread_t *cur;
	unsigned long flags;

	ASSERT(current()->state == THREAD_STATE_RUNNING);

	flags = local_irq_save();

	current()->state = THREAD_STATE_ZOMBIE;

	cur = (queue_thread_t *) malloc(sizeof(queue_thread_t));
	BUG_ON(cur == NULL);

	cur->tcb = current();

	/* Insert the thread at the end of the list */
	list_add_tail(&cur->list, &zombieThreads);

	local_irq_restore(flags);

	schedule();
}

/*
 * Remove a thread from the zombie list.
 */
void remove_zombie(struct tcb *tcb) {
	queue_thread_t *cur;
	tcb_t *_tcb;
	struct list_head *pos, *q;

	ASSERT(local_irq_is_disabled());

	ASSERT(tcb != NULL);
	ASSERT(tcb->state == THREAD_STATE_ZOMBIE);

	list_for_each_safe(pos, q, &zombieThreads)
	{
		cur = list_entry(pos, queue_thread_t, list);

		_tcb = cur->tcb;

		if (tcb == _tcb) {
			list_del(pos);

			free(cur);
			return ;
		}

	}

	printk("%s: zombie thread %d not found.\n", __func__, tcb->tid);

	BUG();
}

/*
 * Wake up a thread which is in waiting state.
 * If the thread passed as argument is not sleeping, we just skip it.
 */
void wake_up(struct tcb *tcb) {
	unsigned long flags;

	flags = spin_lock_irqsave(&schedule_lock);

	if (tcb->state == THREAD_STATE_WAITING)
		ready(tcb);

	spin_unlock_irqrestore(&schedule_lock, flags);
}

/*
 * Remove a tcb from the ready list
 */
void remove_ready(struct tcb *tcb) {
	queue_thread_t *cur;
	tcb_t *_tcb;
	struct list_head *pos, *q;

	ASSERT(local_irq_is_disabled());
	ASSERT(tcb != NULL);

	ASSERT(tcb->state == THREAD_STATE_READY);

	spin_lock(&schedule_lock);

	list_for_each_safe(pos, q, &readyThreads)
	{
		cur = list_entry(pos, queue_thread_t, list);
		_tcb = cur->tcb;

		if (tcb == _tcb) {
			list_del(pos);

			free(cur);

			spin_unlock(&schedule_lock);
			return ;
		}

	}

	printk("%s: ready thread %d not found.\n", __func__, tcb->tid);

	BUG();
}

#ifdef CONFIG_SCHED_PRIO_DYN
/*
 * Pick up the next ready thread to be scheduled according
 * to the scheduling policy. Increment priority if the thread is in the ready
 * list for too long.
 * IRQs are off.
 */
static tcb_t *next_thread(void) {
	tcb_t *tcb, *__current;
	queue_thread_t *entry;
	struct list_head *pos, *q;
	u64 current_time;

	tcb_t *tcb_to_schedule = NULL;
	__current = current();

	/* Check if the current thread is still in the running state, otherwise we skip it */
	if ((__current != NULL) && (__current->state != THREAD_STATE_RUNNING))
		__current = NULL;

	ASSERT(local_irq_is_disabled());

	spin_lock(&schedule_lock);

	/* Increase priority for every ready threads left */
	list_for_each_safe(pos, q, &readyThreads)
	{
		entry = list_entry(pos, queue_thread_t, list);
		tcb = entry->tcb;

		current_time = NOW();
		if (tcb->last_prio_inc_time + MILLISECS(PRIO_MAX_DELAY) < current_time) {
			if (tcb->current_prio < 99)
				tcb->current_prio++;

			tcb->last_prio_inc_time = current_time;
		}
	}

	tcb_to_schedule = __current;
	list_for_each_safe(pos, q, &readyThreads)
	{
		entry = list_entry(pos, queue_thread_t, list);
		tcb = entry->tcb;

		/* First, compare with the running tcb if any... */
		if (!tcb_to_schedule)
			tcb_to_schedule = tcb;
	        else if (tcb->current_prio > tcb_to_schedule->current_prio)
			tcb_to_schedule = tcb;
	}

	spin_unlock(&schedule_lock);

	if (tcb_to_schedule != __current)
		remove_ready(tcb_to_schedule);

	return tcb_to_schedule;
}
#endif /* CONFIG_SCHED_PRIO_DYN */

#ifdef CONFIG_SCHED_PRIO
/*
 * Pick up the next ready thread to be scheduled according
 * to the scheduling policy.
 * IRQs are off.
 */
static tcb_t *next_thread(void) {
	tcb_t *tcb, *__current;
	queue_thread_t *entry;
	struct list_head *pos, *q;

	tcb_t *tcb_to_schedule = NULL;
	__current = current();

	/* Check if the current thread is still in the running state, otherwise we skip it */
	if ((__current != NULL) && (__current->state != THREAD_STATE_RUNNING))
		__current = NULL;

	ASSERT(local_irq_is_disabled());

	spin_lock(&schedule_lock);

	tcb_to_schedule = __current;
	list_for_each_safe(pos, q, &readyThreads)
	{
		entry = list_entry(pos, queue_thread_t, list);
		tcb = entry->tcb;

		/* First, compare with the running tcb if any... */
		if (!tcb_to_schedule)
			tcb_to_schedule = tcb;
	    else if (tcb->prio > tcb_to_schedule->prio)
	        tcb_to_schedule = tcb;

	}

	spin_unlock(&schedule_lock);

	if (tcb_to_schedule != __current)
		remove_ready(tcb_to_schedule);

	return tcb_to_schedule;
}
#endif /* CONFIG_SCHED_PRIO */

#ifdef CONFIG_SCHED_RR
/*
 * Pick up the next ready thread to be scheduled according
 * to the scheduling policy.
 * IRQs are off.
 */
static tcb_t *next_thread(void) {
	tcb_t *tcb;
	queue_thread_t *entry;
	struct list_head *pos, *q;

	ASSERT(local_irq_is_disabled());

	spin_lock(&schedule_lock);

	list_for_each_safe(pos, q, &readyThreads)
	{
		entry = list_entry(pos, queue_thread_t, list);
		tcb = entry->tcb;

		if ((tcb->pcb == NULL) || (tcb->pcb->state == PROC_STATE_READY) || (tcb->pcb->state == PROC_STATE_RUNNING)) {
			spin_unlock(&schedule_lock);

			/* Warning ! entry will be freed in remove_ready() */
			remove_ready(tcb);

			return tcb;
		}
	}

	spin_unlock(&schedule_lock);

	return NULL;
}
#endif /* CONFIG_SCHED_RR */

/*
 * Main scheduling function.
 */
void schedule(void) {
	tcb_t *prev, *next;
	unsigned long flags;
	static volatile bool __in_scheduling = false;

	if (unlikely(boot_stage < BOOT_STAGE_COMPLETED))
		return ;

	BUG_ON(!__sched_preempt);
	BUG_ON(!tcb_idle);

	/* Already scheduling? May happen if set_timer leads to another softirq schedule */
	if (__in_scheduling)
		return ;

	flags = local_irq_save();

	__in_scheduling = true;

	/* Scheduling policy: at the moment start the first ready thread */

	prev = current();
	next = next_thread();

#ifdef CONFIG_SCHED_FREQ_PREEMPTION
	set_timer(&schedule_timer, NOW() + MILLISECS(SCHEDULE_FREQ));
#endif

	/* prev may be NULL at the very beginning (current is set to NULL at init). */
	if ((next == NULL) && (!prev || prev->state != THREAD_STATE_RUNNING))
		next = tcb_idle;

	if (next && (next != prev)) {

		DBG("Now scheduling thread ID: %d name: %s PID: %d prio: %d\n", next->tid, next->name, ((next->pcb != NULL) ? next->pcb->pid : -1), next->prio);

		/*
		 * The current threads (here prev) can be in different states, not only running; it may be in *waiting* or *zombie*
		 * depending on the thread activities. Hence, we put it in the ready state ONLY if the thread is in *running*.
		 */
		if ((prev != NULL) && (prev->state == THREAD_STATE_RUNNING) && (likely(prev != tcb_idle)))
			ready(prev);

		next->state = THREAD_STATE_RUNNING;
		set_current(next);

#ifdef CONFIG_MMU
		if ((next->pcb != NULL) && (next->pcb->pgtable != current_pgtable())) {

			/* Change the process context */

			next->pcb->state = PROC_STATE_RUNNING;

			if ((prev != NULL) && (prev->pcb != NULL) && (prev->pcb->state != PROC_STATE_ZOMBIE) && (prev->pcb->state != PROC_STATE_WAITING))
				prev->pcb->state = PROC_STATE_READY;

			mmu_switch((void *) __pa(next->pcb->pgtable));
			set_pgtable(next->pcb->pgtable);

		}
#endif /* CONFIG_MMU */

		/* Authorized to leave the interrupt context here */
		__in_interrupt = false;
		__in_scheduling = false;

		__switch_to(prev, next);

	}
	__in_scheduling = false;
	__in_interrupt = false;

	local_irq_restore(flags);
}

/*
 * Can be used for debugging purposes.
 *
 */
void __dump_regs(unsigned long regs) {
	unsigned long *cpuregs = (unsigned long *) regs;

	lprintk("r4: %x ", *cpuregs);
	lprintk("r5: %x ", *(cpuregs+1));
	lprintk("r6: %x ", *(cpuregs+2));
	lprintk("r7: %x ", *(cpuregs+3));
	lprintk("r8: %x ", *(cpuregs+4));
	lprintk("r9: %x ", *(cpuregs+5));
	lprintk("r10: %x ", *(cpuregs+6));
	lprintk("fp: %x ", *(cpuregs+7));
	lprintk("ip: %x ", *(cpuregs+8));
	lprintk("sp: %x ", *(cpuregs+9));
	lprintk("lr: %x ", *(cpuregs+10));
	lprintk("pc: %x ", *(cpuregs+11));
	lprintk("psr: %x ", *(cpuregs+12));
	lprintk("\n");
}

static inline void raise_schedule(void *__dummy) {
	raise_softirq(SCHEDULE_SOFTIRQ);
}

/*
 * Dump the ready threads
 */
void dump_ready(void) {
	struct list_head *pos;
	queue_thread_t *cur;
	unsigned long flags;

	lprintk("Dumping the ready-threads queue: \n");

	flags = local_irq_save();

	if (list_empty(&readyThreads)) {
		lprintk("  <empty>\n");
		local_irq_restore(flags);
		return ;
	}

	list_for_each(pos, &readyThreads)
	{
		cur = list_entry(pos, queue_thread_t, list);
		lprintk("  Thread ID: %d name: %s state: %d prio: %d\n", cur->tcb->tid, cur->tcb->name, cur->tcb->state, cur->tcb->prio);

	}

	local_irq_restore(flags);
}

/*
 * Dump the zombie threads
 */
void dump_zombie(void) {
	struct list_head *pos;
	queue_thread_t *cur;
	unsigned long flags;

	printk("Dumping the zombie-threads queue: \n");

	flags = local_irq_save();

	if (list_empty(&zombieThreads)) {
		printk("  <empty>\n");
		local_irq_restore(flags);
		return ;
	}

	list_for_each(pos, &zombieThreads)
	{
		cur = list_entry(pos, queue_thread_t, list);
		printk("  Proc ID: %d Thread ID: %d state: %d\n", ((cur->tcb->pcb != NULL) ? cur->tcb->pcb->pid : 0), cur->tcb->tid, cur->tcb->state);

	}

	local_irq_restore(flags);
}

/*
 * Dump information of the scheduling waitqueues.
 */
void dump_sched(void) {
	dump_ready();
	dump_zombie();
}

void scheduler_init(void) {

	boot_stage = BOOT_STAGE_SCHED;

	/* Low-level thread initialization */
	threads_init();

	/* Initialization of the scheduling policy */
#if defined(CONFIG_SCHED_PRIO_DYN)
	sched_policy = SCHED_POLICY_PRIO_DYN;
#elif defined(CONFIG_SCHED_PRIO)
	sched_policy = SCHED_POLICY_PRIO;
#elif defined(CONFIG_SCHED_RR)
	sched_policy = SCHED_POLICY_RR;
#endif

	/* Initialize the main queues addressed by the scheduler */
	INIT_LIST_HEAD(&readyThreads);
	INIT_LIST_HEAD(&zombieThreads);

	/* Initialize the global list of processes */
	INIT_LIST_HEAD(&proc_list);

	spin_lock_init(&schedule_lock);

	/* Registering our softirq to activate the scheduler when necessary */
	register_softirq(SCHEDULE_SOFTIRQ, schedule);

	/* Initialize the idle thread so that we make sure there is at least
	 * one ready thread, and support other threads to be waiting at
	 * their early execution.
	 */

	/* Start the idle thread with priority 1. */
	tcb_idle = kernel_thread(thread_idle, "idle", NULL, 1);

	/* We put the current thread as NULL. The scheduler will avoid
	 * to preserve hazardous register for a running thread which
	 * has not been scheduled by itself. Therefore, the idle thread
	 * starts to be ready first before getting running.
	 */
	set_current(NULL);

	preempt_enable();

	/* Initiate a timer to trigger the schedule function */
	init_timer(&schedule_timer, raise_schedule, NULL, smp_processor_id());

	set_timer(&schedule_timer, NOW() + MILLISECS(SCHEDULE_FREQ));
}
