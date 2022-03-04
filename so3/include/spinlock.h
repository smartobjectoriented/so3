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
#include <asm/spinlock.h>

#define DEFINE_SPINLOCK(l) spinlock_t l = { 0 };

#define spin_lock_init_prof(s, l) spin_lock_init(&((s)->l))
#define lock_profile_register_struct(type, ptr, idx, print)
#define lock_profile_deregister_struct(type, ptr)

typedef struct {
    raw_spinlock_t raw;
} spinlock_t;

#define spin_lock_init(l) (*(l) = (spinlock_t){ 0 })

void spin_lock(spinlock_t *lock);
void spin_lock_irq(spinlock_t *lock);
uint32_t spin_lock_irqsave(spinlock_t *lock);

void spin_unlock(spinlock_t *lock);
void spin_unlock_irq(spinlock_t *lock);
void spin_unlock_irqrestore(spinlock_t *lock, uint32_t flags);

int spin_is_locked(spinlock_t *lock);
int spin_trylock(spinlock_t *lock);


#endif /* SPINLOCK_H */

