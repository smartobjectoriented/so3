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

/*
 * This implementation of mutex is inspired from Linux source code.
 */

#ifndef MUTEX_H
#define MUTEX_H

#include <list.h>
#include <spinlock.h>
#include <thread.h>

#include <asm/atomic.h>

struct mutex {
   /* 1: unlocked, 0: locked, negative: locked, possible waiters */
   atomic_t                count;
   tcb_t		   *owner;
   spinlock_t              wait_lock;

   /* Allow to manage recursive locking */
   uint32_t 		   recursive_count;

   struct list_head        tcb_list;
};
typedef struct mutex mutex_t;

 /**
  * mutex_is_locked - is the mutex locked
  * @lock: the mutex to be queried
  *
  * Returns 1 if the mutex is locked, 0 if unlocked.
  */
 static inline int mutex_is_locked(struct mutex *lock)
 {
 	return atomic_read(&lock->count) != 1;
 }

void mutex_lock(struct mutex *lock);
void mutex_unlock(struct mutex *lock);
void mutex_init(struct mutex *lock);

int do_mutex_init(void);
int do_mutex_lock(mutex_t *lock);
int do_mutex_unlock(mutex_t *lock);

#endif /* MUTEX_H */

