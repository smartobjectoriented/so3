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

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <list.h>
#include <thread.h>

/* SCHEDULE_FREQ is the scheduler tick expressed in ms */
#define SCHEDULE_FREQ	10

#ifdef CONFIG_SCHED_PRIO_DYN

/* Maximum delay in ms in ready state */
#define PRIO_MAX_DELAY 500

#endif /* CONFIG_SCHED_PRIO_DYN */

extern volatile u64 jiffies;
extern volatile u64 jiffies_ref;

extern struct tcb *tcb_idle;

void scheduler_init(void);
void scheduler_start(void);

void schedule(void);

void preempt_enable(void);
void preempt_disable(void);

void ready(struct tcb *tcb);
void waiting(void);
struct tcb *pick_waiting_thread(struct tcb *tcb);
void zombie(void);

void remove_zombie(struct tcb *tcb);

void wake_up(struct tcb *tcb);

void dump_sched(void);

extern struct tcb *current_thread;

static inline void set_current(struct tcb *tcb) {
	current_thread = tcb;
}

static inline struct tcb *current(void) {
	return current_thread;
}

struct tcb *current(void);

static inline void reset_thread_timeout(void) {
	current_thread->timeout = 0ull;
}

void remove_ready(struct tcb *tcb);

void dump_ready(void);

void schedule_isr(void);

typedef struct queue_thread {
	struct list_head list;
	struct tcb *tcb;
} queue_thread_t;

typedef enum {
	SCHED_POLICY_RR = 0x1,
	SCHED_POLICY_PRIO = 0x2,
	SCHED_POLICY_PRIO_DYN = 0x3
} sched_policy_t;

#endif /* SCHEDULE_H */
