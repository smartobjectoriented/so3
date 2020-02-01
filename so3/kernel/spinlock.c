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

static atomic_t spin_debug = ATOMIC_INIT(0);

static void check_lock(struct lock_debug *debug)
{
	int irq_safe = !local_irq_is_enabled();

	if (unlikely(atomic_read(&spin_debug) <= 0))
		return;

	/* A few places take liberties with this. */
	/* BUG_ON(in_irq() && !irq_safe); */

	if (unlikely(debug->irq_safe != irq_safe))
	{
		int seen = cmpxchg(&debug->irq_safe, -1, irq_safe);
		BUG_ON(seen == !irq_safe);
	}
}

void spin_debug_enable(void)
{
	atomic_inc(&spin_debug);
}

void spin_debug_disable(void)
{
	atomic_dec(&spin_debug);
}


void _spin_lock(spinlock_t *lock)
{
	check_lock(&lock->debug);
	while (unlikely(!_raw_spin_trylock(&lock->raw)))
	{

		while (likely(_raw_spin_is_locked(&lock->raw)))
			cpu_relax();
	}
}

void _spin_lock_irq(spinlock_t *lock)
{
	local_irq_disable();
	check_lock(&lock->debug);
	while (unlikely(!_raw_spin_trylock(&lock->raw)))
	{

		local_irq_enable();
		while (likely(_raw_spin_is_locked(&lock->raw)))
			cpu_relax();
		local_irq_disable();
	}

}

uint32_t _spin_lock_irqsave(spinlock_t *lock)
{
	uint32_t flags;

	flags = local_irq_save();

	check_lock(&lock->debug);
	while (unlikely(!_raw_spin_trylock(&lock->raw)))
	{

		local_irq_restore(flags);
		while (likely(_raw_spin_is_locked(&lock->raw)))
			cpu_relax();
		flags = local_irq_save();
	}

	return flags;
}

void _spin_unlock(spinlock_t *lock)
{
	_raw_spin_unlock(&lock->raw);
}

void _spin_unlock_irq(spinlock_t *lock)
{
	_raw_spin_unlock(&lock->raw);
	local_irq_enable();
}

void _spin_unlock_irqrestore(spinlock_t *lock, uint32_t flags)
{
	_raw_spin_unlock(&lock->raw);
	local_irq_restore(flags);
}

int _spin_is_locked(spinlock_t *lock)
{
	check_lock(&lock->debug);
	return _raw_spin_is_locked(&lock->raw);
}

int _spin_trylock(spinlock_t *lock)
{
	check_lock(&lock->debug);

	return _raw_spin_trylock(&lock->raw);

}

void _spin_barrier(spinlock_t *lock)
{

	check_lock(&lock->debug);
	do { mb(); } while ( _raw_spin_is_locked(&lock->raw) );

	mb();
}

void _spin_barrier_irq(spinlock_t *lock)
{
	uint32_t flags;
	flags = local_irq_save();
	_spin_barrier(lock);
	local_irq_restore(flags);
}

void _spin_lock_recursive(spinlock_t *lock)
{
	int cpu = smp_processor_id();

	/* Don't allow overflow of recurse_cpu field. */
	BUILD_BUG_ON(NR_CPUS > 0xfffu);

	check_lock(&lock->debug);

	if ( likely(lock->recurse_cpu != cpu) )
	{
		spin_lock(lock);
		lock->recurse_cpu = cpu;
	}

	/* We support only fairly shallow recursion, else the counter overflows. */
	ASSERT(lock->recurse_cnt < 0xfu);
	lock->recurse_cnt++;
}

void _spin_unlock_recursive(spinlock_t *lock)
{
	if ( likely(--lock->recurse_cnt == 0) )
	{
		lock->recurse_cpu = 0xfffu;
		spin_unlock(lock);
	}
}

void _read_lock(rwlock_t *lock)
{
	check_lock(&lock->debug);
	_raw_read_lock(&lock->raw);
}

void _read_lock_irq(rwlock_t *lock)
{
	ASSERT(local_irq_is_enabled());
	local_irq_disable();
	check_lock(&lock->debug);
	_raw_read_lock(&lock->raw);
}

uint32_t _read_lock_irqsave(rwlock_t *lock)
{
	uint32_t flags;

	flags = local_irq_save();
	check_lock(&lock->debug);
	_raw_read_lock(&lock->raw);
	return flags;
}

void _read_unlock(rwlock_t *lock)
{
	_raw_read_unlock(&lock->raw);
}

void _read_unlock_irq(rwlock_t *lock)
{
	_raw_read_unlock(&lock->raw);
	local_irq_enable();
}

void _read_unlock_irqrestore(rwlock_t *lock, uint32_t flags)
{
	_raw_read_unlock(&lock->raw);
	local_irq_restore(flags);
}

void _write_lock(rwlock_t *lock)
{
	check_lock(&lock->debug);
	_raw_write_lock(&lock->raw);
}

void _write_lock_irq(rwlock_t *lock)
{
	ASSERT(local_irq_is_enabled());
	local_irq_disable();
	check_lock(&lock->debug);
	_raw_write_lock(&lock->raw);
}

uint32_t _write_lock_irqsave(rwlock_t *lock)
{
	uint32_t flags;
	flags = local_irq_save();
	check_lock(&lock->debug);
	_raw_write_lock(&lock->raw);
	return flags;
}

int _write_trylock(rwlock_t *lock)
{
	check_lock(&lock->debug);
	return _raw_write_trylock(&lock->raw);
}

void _write_unlock(rwlock_t *lock)
{
	_raw_write_unlock(&lock->raw);
}

void _write_unlock_irq(rwlock_t *lock)
{
	_raw_write_unlock(&lock->raw);
	local_irq_enable();
}

void _write_unlock_irqrestore(rwlock_t *lock, uint32_t flags)
{
	_raw_write_unlock(&lock->raw);
	local_irq_restore(flags);
}

int _rw_is_locked(rwlock_t *lock)
{
	check_lock(&lock->debug);
	return _raw_rw_is_locked(&lock->raw);
}

int _rw_is_write_locked(rwlock_t *lock)
{
	check_lock(&lock->debug);
	return _raw_rw_is_write_locked(&lock->raw);
}
