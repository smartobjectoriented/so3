/*
 * Copyright (C) 2014-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <spinlock.h>
#include <compiler.h>

#include <asm/processor.h>
#include <asm/atomic.h>

#include <device/irq.h>
#include <device/serial.h>

void spin_lock(spinlock_t *lock)
{
	while (unlikely(!spin_trylock(lock)))
	{
		while (likely(spin_is_locked(lock))) {
			cpu_relax();
		}
	}
}

void spin_lock_irq(spinlock_t *lock)
{
	local_irq_disable();

	while (unlikely(!spin_trylock(lock)))
	{

		local_irq_enable();
		while (likely(spin_is_locked(lock)))
			cpu_relax();
		local_irq_disable();
	}

}

unsigned long spin_lock_irqsave(spinlock_t *lock)
{
	unsigned long flags;

	flags = local_irq_save();

	while (unlikely(!spin_trylock(lock)))
	{

		local_irq_restore(flags);
		while (likely(spin_is_locked(lock)))
			cpu_relax();

		flags = local_irq_save();
	}

	return flags;
}

void spin_unlock_irq(spinlock_t *lock)
{
	spin_unlock(lock);

	local_irq_enable();
}

void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
	spin_unlock(lock);

	local_irq_restore(flags);
}


void spin_barrier(spinlock_t *lock)
{
	do { smp_mb(); } while (spin_is_locked(lock));

	smp_mb();
}
