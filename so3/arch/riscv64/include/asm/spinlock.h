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

#ifndef ASM_SPINLOCK_H
#define ASM_SPINLOCK_H

#include <asm/atomic.h>


typedef struct {
	volatile unsigned int lock;
} raw_spinlock_t;

#define _RAW_SPIN_LOCK_UNLOCKED	{ 0 }

typedef struct {
	volatile unsigned int lock  __attribute__((__packed__));
} raw_rwlock_t;


/*
 * ARMv6 Spin-locking.
 *
 * We exclusively read the old value.  If it is zero, we may have
 * won the lock, so we try exclusively storing it.  A memory barrier
 * is required after we get a lock, and before we release it, because
 * V6 CPUs are assumed to have weakly ordered memory.
 *
 * Unlocked value: 0
 * Locked value: 1
 */

#define _raw_spin_is_locked(x)		((x)->lock != 0)
#define __raw_spin_unlock_wait(lock) \
	do { while (__raw_spin_is_locked(lock)) cpu_relax(); } while (0)

#define __raw_spin_lock_flags(lock, flags) __raw_spin_lock(lock)

static inline void __raw_spin_lock(raw_spinlock_t *lock)
{

}

static inline int _raw_spin_trylock(raw_spinlock_t *lock)
{

}

static inline void _raw_spin_unlock(raw_spinlock_t *lock)
{

}

/*
 * RWLOCKS
 *
 *
 * Write locks are easy - we just set bit 31.  When unlocking, we can
 * just write zero since the lock is exclusively held.
 */
#define rwlock_is_locked(x)	(*((volatile unsigned int *)(x)) != 0)

static inline void _raw_write_lock(raw_rwlock_t *rw)
{

}

static inline int _raw_write_trylock(raw_rwlock_t *rw)
{

}

static inline void _raw_write_unlock(raw_rwlock_t *rw)
{

}

/* write_can_lock - would write_trylock() succeed? */

#define __raw_write_can_lock(x)		((x)->lock == 0x80000000)
#define RW_LOCK_BIAS 0x01000000
#define _RAW_RW_LOCK_UNLOCKED /*(raw_rwlock_t)*/ { RW_LOCK_BIAS }

/*
 * Read locks are a bit more hairy:
 *  - Exclusively load the lock value.
 *  - Increment it.
 *  - Store new lock value if positive, and we still own this location.
 *    If the value is negative, we've already failed.
 *  - If we failed to store the value, we want a negative result.
 *  - If we failed, try again.
 * Unlocking is similarly hairy.  We may have multiple read locks
 * currently active.  However, we know we won't have any write
 * locks.
 */
static inline void _raw_read_lock(raw_rwlock_t *rw)
{

}

static inline void _raw_read_unlock(raw_rwlock_t *rw)
{

}

static inline int _raw_read_trylock(raw_rwlock_t *rw)
{

}

/* read_can_lock - would read_trylock() succeed? */
#define __raw_read_can_lock(x)		((x)->lock < 0x80000000)

#define _raw_rw_is_locked(x) ((x)->lock < RW_LOCK_BIAS)
#define _raw_rw_is_write_locked(x) ((x)->lock <= 0)

/* Per-domain lock can be recursively acquired in fault handlers. */
#define LOCK_BIGLOCK(_d) spin_lock_recursive(&(_d)->domain_lock)
#define UNLOCK_BIGLOCK(_d) spin_unlock_recursive(&(_d)->domain_lock)


#endif /* ASM_SPINLOCK_H */
