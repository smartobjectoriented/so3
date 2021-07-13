/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2015 Regents of the University of California
 * Copyright (C) 2017 SiFive
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

#include <types.h>

#include <asm/atomic.h>


typedef struct {
	volatile unsigned int lock;
} raw_spinlock_t;

#define _RAW_SPIN_LOCK_UNLOCKED	{ 0 }

typedef struct {
	volatile unsigned int lock  __attribute__((__packed__));
} raw_rwlock_t;

#if 0
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

#endif

#define RISCV_ACQUIRE_BARRIER		"\tfence r , rw\n"
#define RISCV_RELEASE_BARRIER		"\tfence rw,  w\n"

#define _raw_spin_is_locked(x)		((x)->lock != 0)

static inline void _raw_spin_unlock(raw_spinlock_t *lock)
{
	smp_store_release(&lock->lock, 0);
}

static inline int _raw_spin_trylock(raw_spinlock_t *lock)
{
	int tmp = 1, busy;

	__asm__ __volatile__ (
		"	amoswap.w %0, %2, %1\n"
		RISCV_ACQUIRE_BARRIER
		: "=r" (busy), "+A" (lock->lock)
		: "r" (tmp)
		: "memory");

	return !busy;
}

static inline void _raw_spin_lock(raw_spinlock_t *lock)
{
	while (1) {
		if (_raw_spin_is_locked(lock))
			continue;

		if (_raw_spin_trylock(lock))
			break;
	}
}

static inline void _raw_read_lock(raw_rwlock_t *lock)
{
	int tmp;

	__asm__ __volatile__(
		"1:	lr.w	%1, %0\n"
		"	bltz	%1, 1b\n"
		"	addi	%1, %1, 1\n"
		"	sc.w	%1, %1, %0\n"
		"	bnez	%1, 1b\n"
		RISCV_ACQUIRE_BARRIER
		: "+A" (lock->lock), "=&r" (tmp)
		:: "memory");
}

static inline void _raw_write_lock(raw_rwlock_t *lock)
{
	int tmp;

	__asm__ __volatile__(
		"1:	lr.w	%1, %0\n"
		"	bnez	%1, 1b\n"
		"	li	%1, -1\n"
		"	sc.w	%1, %1, %0\n"
		"	bnez	%1, 1b\n"
		RISCV_ACQUIRE_BARRIER
		: "+A" (lock->lock), "=&r" (tmp)
		:: "memory");
}

static inline int _raw_read_trylock(raw_rwlock_t *lock)
{
	int busy;

	__asm__ __volatile__(
		"1:	lr.w	%1, %0\n"
		"	bltz	%1, 1f\n"
		"	addi	%1, %1, 1\n"
		"	sc.w	%1, %1, %0\n"
		"	bnez	%1, 1b\n"
		RISCV_ACQUIRE_BARRIER
		"1:\n"
		: "+A" (lock->lock), "=&r" (busy)
		:: "memory");

	return !busy;
}

static inline int _raw_write_trylock(raw_rwlock_t *lock)
{
	int busy;

	__asm__ __volatile__(
		"1:	lr.w	%1, %0\n"
		"	bnez	%1, 1f\n"
		"	li	%1, -1\n"
		"	sc.w	%1, %1, %0\n"
		"	bnez	%1, 1b\n"
		RISCV_ACQUIRE_BARRIER
		"1:\n"
		: "+A" (lock->lock), "=&r" (busy)
		:: "memory");

	return !busy;
}

static inline void _raw_read_unlock(raw_rwlock_t *lock)
{
	__asm__ __volatile__(
		RISCV_RELEASE_BARRIER
		"	amoadd.w x0, %1, %0\n"
		: "+A" (lock->lock)
		: "r" (-1)
		: "memory");
}

static inline void _raw_write_unlock(raw_rwlock_t *lock)
{
	smp_store_release(&lock->lock, 0);
}

/* write_can_lock - would write_trylock() succeed? */

#define __raw_write_can_lock(x)		((x)->lock == 0x80000000)
#define RW_LOCK_BIAS 0x01000000
#define _RAW_RW_LOCK_UNLOCKED /*(raw_rwlock_t)*/ { RW_LOCK_BIAS }

/* read_can_lock - would read_trylock() succeed? */
#define __raw_read_can_lock(x)		((x)->lock < 0x80000000)

#define _raw_rw_is_locked(x) ((x)->lock < RW_LOCK_BIAS)
#define _raw_rw_is_write_locked(x) ((x)->lock <= 0)

#endif /* ASM_SPINLOCK_H */
