/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

static inline int spin_trylock(spinlock_t *lock)
{
	return !__atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE);
}

static inline void spin_unlock(spinlock_t *lock)
{
	__atomic_store_n(&lock->lock, 0, __ATOMIC_RELEASE);
}

#endif /* ASM_SPINLOCK_H */
