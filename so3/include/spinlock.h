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

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <asm/processor.h>

/*
 * On Aarch64, the field has to be 64-bit aligned apparently.
 */
typedef struct {
	__attribute__ ((aligned (8))) volatile uint32_t lock;
} spinlock_t;

#include <asm/spinlock.h>

#define DEFINE_SPINLOCK(l) spinlock_t l = { 0 };

#define spin_lock_init(l) (*(l) = (spinlock_t){ 0 })

void spin_lock(spinlock_t *lock);
void spin_lock_irq(spinlock_t *lock);
unsigned long spin_lock_irqsave(spinlock_t *lock);

void spin_unlock(spinlock_t *lock);
void spin_unlock_irq(spinlock_t *lock);
void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags);

#define spin_is_locked(x)	((x)->lock != 0)

void spin_barrier(spinlock_t *lock);

#endif /* SPINLOCK_H */

