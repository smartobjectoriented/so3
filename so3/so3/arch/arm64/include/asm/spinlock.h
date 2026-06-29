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

static inline int spin_trylock(spinlock_t *lock)
{
	uint32_t tmp;

	/* The spinlock must be 64-bit aligned in aarch64 */
	BUG_ON((((uint64_t) &lock->lock) & 0x7) != 0);

	__asm__ __volatile__("	ldaxr	%x0, [%1]\n"
			     "	cbnz	%x0, 1f\n"
			     "	stxr	%w0, %2, [%1]\n"
			     "1:"
			     : "=&r"(tmp)
			     : "r"(&lock->lock), "r"(1ULL)
			     : "cc", "memory");

	if (tmp == 0) {
		smp_mb();
		return 1;
	} else {
		return 0;
	}
}

/*
 * Release lock previously acquired by spin_lock.
 *
 * Use store-release to unconditionally clear the spinlock variable.
 * Store operation generates an event to all cores waiting in WFE
 * when address is monitored by the global monitor.
 *
 * void spin_unlock(spinlock_t *lock);
 */
static inline void spin_unlock(spinlock_t *lock)
{
	smp_mb();

	__asm__ __volatile__("	stlr	xzr, [%0]\n"
			     "	sev"
			     :
			     : "r"(&lock->lock)
			     : "cc", "memory");
}
#endif /* ASM_SPINLOCK_H */
