/*
 * Copyright (C) 2020 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <mutex.h>
#include <list.h>
#include <timer.h>

#include <asm/atomic.h>

typedef struct {

	/* Semaphore counter */
	uint32_t val;

	/* Used to manage atomic operation during sleep/wakeup operation */
	/* 1: unlocked, 0: locked, negative: locked, possible waiters */
	atomic_t count;

	/* Protect access to internal variables */
	mutex_t lock;

	/* Waiting queue */
	struct list_head tcb_list;

} sem_t;

void sem_up(sem_t *sem);
void sem_down(sem_t *sem);
int sem_timeddown(sem_t *sem, uint64_t timeout);

void sem_init(sem_t *sem);

#endif /* SEMAPHORE_H */

