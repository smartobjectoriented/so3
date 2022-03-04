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

#include <spinlock.h>
#include <asm/processor.h>
#include <asm/atomic.h>
#include <compiler.h>

#include <device/irq.h>
#include <device/serial.h>

void spin_lock(spinlock_t *lock)
{
	while (unlikely(!_raw_spin_trylock(&lock->raw)))
	{

		while (likely(_raw_spin_is_locked(&lock->raw)))
			cpu_relax();
	}
}

void spin_lock_irq(spinlock_t *lock)
{
	local_irq_disable();

	while (unlikely(!_raw_spin_trylock(&lock->raw)))
	{

		local_irq_enable();
		while (likely(_raw_spin_is_locked(&lock->raw)))
			cpu_relax();
		local_irq_disable();
	}

}

uint32_t spin_lock_irqsave(spinlock_t *lock)
{
	uint32_t flags;

	flags = local_irq_save();

	while (unlikely(!_raw_spin_trylock(&lock->raw)))
	{

		local_irq_restore(flags);
		while (likely(_raw_spin_is_locked(&lock->raw)))
			cpu_relax();
		flags = local_irq_save();
	}

	return flags;
}

void spin_unlock(spinlock_t *lock)
{
	_raw_spin_unlock(&lock->raw);
}

void spin_unlock_irq(spinlock_t *lock)
{
	_raw_spin_unlock(&lock->raw);
	local_irq_enable();
}

void spin_unlock_irqrestore(spinlock_t *lock, uint32_t flags)
{
	_raw_spin_unlock(&lock->raw);
	local_irq_restore(flags);
}

int spin_is_locked(spinlock_t *lock)
{
	return _raw_spin_is_locked(&lock->raw);
}

int spin_trylock(spinlock_t *lock)
{
	return _raw_spin_trylock(&lock->raw);

}

