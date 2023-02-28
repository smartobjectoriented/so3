/*
 * Copyright (C) 2014-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef BITOPS_H
#define BITOPS_H

#include <types.h>

#include <asm/processor.h>

#define __BIT_MASK(nr)		(1UL << ((nr) % BITS_PER_INT))
#define BIT_WORD(nr)		((nr) / BITS_PER_INT)

extern unsigned long _find_first_zero_bit(const unsigned long *p, unsigned long size);
extern unsigned long _find_next_zero_bit(const unsigned long *p, unsigned long size, unsigned long offset);
extern unsigned long _find_first_bit(const unsigned long *p, unsigned long size);
extern unsigned long _find_next_bit(const unsigned long *p, unsigned long size, unsigned long offset);

#define find_first_zero_bit(p, sz)       _find_first_zero_bit(p, sz)
#define find_next_zero_bit(p, sz, off)    _find_next_zero_bit(p, sz, off)
#define find_first_bit(p, sz)            _find_first_bit(p, sz)
#define find_next_bit(p, sz, off)         _find_next_bit(p, sz, off)

/*
 * These functions are the basis of our bit ops.
 *
 * First, the atomic bitops. These use native endian.
 */
static inline void ____atomic_set_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	flags = local_irq_save();
	*p |= mask;
	local_irq_restore(flags);
}

static inline void ____atomic_clear_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	flags = local_irq_save();
	*p &= ~mask;
	local_irq_restore(flags);
}

#define set_bit(nr,p)                   ____atomic_set_bit(nr, p)
#define clear_bit(nr,p)                 ____atomic_clear_bit(nr, p)

extern void change_bit(int nr, volatile void * addr);

static inline void __change_bit(int nr, volatile void *addr)
{
	unsigned long mask = __BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

	*p ^= mask;
}

static inline int __test_and_set_bit(int nr, volatile void *addr)
{
	unsigned long mask = __BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old = *p;

	*p = old | mask;
	return (old & mask) != 0;
}

static inline int test_and_set_bit(int nr, volatile void * addr)
{
	unsigned long flags;
	int out;

	flags = local_irq_save();
	out = __test_and_set_bit(nr, addr);
	local_irq_restore(flags);

	return out;
}

static inline int __test_and_clear_bit(int nr, volatile void *addr)
{
	unsigned long mask = __BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old = *p;

	*p = old & ~mask;
	return (old & mask) != 0;
}

static inline int test_and_clear_bit(int nr, volatile void * addr)
{
	unsigned long flags;
	int out;

	flags = local_irq_save();
	out = __test_and_clear_bit(nr, addr);
	local_irq_restore(flags);

	return out;
}

extern int test_and_change_bit(int nr, volatile void *addr);

static inline int __test_and_change_bit(int nr, volatile void *addr)
{
	unsigned long mask = __BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old = *p;

	*p = old ^ mask;
	return (old & mask) != 0;
}

/*
 * This routine doesn't need to be atomic.
 */
static inline int test_bit(int nr, const volatile void *addr)
{
    return ((unsigned char *) addr)[nr >> 3] & (1U << (nr & 7));
}


#endif /* BITOPS_H */
