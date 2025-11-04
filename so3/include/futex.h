/*
 * Copyright (C) 2014-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef FUTEX_H
#define FUTEX_H

#include <timer.h>
#include <thread.h>
#include <list.h>
#include <spinlock.h>
#include <syscall.h>

/* Commands */
#define FUTEX_WAIT		0
#define FUTEX_WAKE		1
#define FUTEX_FD		2
#define FUTEX_REQUEUE		3
#define FUTEX_CMP_REQUEUE	4
#define FUTEX_WAKE_OP		5
#define FUTEX_LOCK_PI		6
#define FUTEX_UNLOCK_PI		7
#define FUTEX_TRYLOCK_PI	8
#define FUTEX_WAIT_BITSET	9

#define FUTEX_PRIVATE_FLAG 128
#define FUTEX_CLOCK_REALTIME	256
#define FUTEX_CMD_MASK		~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME)

/*
 * Hash buckets are shared by all the futex_keys that hash to the same
 * location.  Each key may have multiple futex_q structures, one for each task
 * waiting on a futex.
 */
// struct futex_hash_bucket {
// 	atomic_t waiters;
// 	spinlock_t lock;
// 	struct plist_head chain;
// };

typedef struct futex_el {
	struct list_head list;
	tcb_t *tcb;
} futex_el_t;

typedef struct futex {
	struct list_head list;
	struct list_head f_element;
	uintptr_t key;
} futex_t;


/**
 * struct futex_q - The hashed futex queue entry, one per waiting task
 *
 * @list:		priority-sorted list of tasks waiting on this futex
 * @lock_ptr:	the hash bucket lock
 * @key:		the key the futex is hashed on
 * @task:		the task waiting on the futex
 */
// struct futex_q {
//     struct plist_node list;
//     struct futex_hash_bucket *lock_ptr;
//     struct futex_key key;
//     struct task_struct *task; // the sleeping thread
// };



SYSCALL_DECLARE(futex, u32 *uaddr, int op, u32 val, const struct timespec * utime,
		u32 *uaddr2, u32 val3)

#endif /* FUTEX_H */
