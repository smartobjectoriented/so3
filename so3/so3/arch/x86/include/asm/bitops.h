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

#ifndef ASM_BITOPS_H
#define ASM_BITOPS_H

#include <compiler.h>
#include <asm/processor.h>

#ifndef __ASSEMBLY__

#define __L2(_x) (((_x) & 0x00000002) ? 1 : 0)
#define __L4(_x) (((_x) & 0x0000000c) ? (2 + __L2((_x) >> 2)) : __L2(_x))
#define __L8(_x) (((_x) & 0x000000f0) ? (4 + __L4((_x) >> 4)) : __L4(_x))
#define __L16(_x) (((_x) & 0x0000ff00) ? (8 + __L8((_x) >> 8)) : __L8(_x))
#define LOG_2(_x) (((_x) & 0xffff0000) ? (16 + __L16((_x) >> 16)) : __L16(_x))

#define BIT_MASK(last, first) ((0xffffffffffffffffULL >> (64 - ((last) + 1 - (first)))) << (first))
#define GET_FIELD(value, last, first) (((value) & BIT_MASK((last), (first))) >> (first))

#define smp_mb__before_clear_bit() mb()
#define smp_mb__after_clear_bit() mb()

#define BITOP_MASK(nr) (1UL << ((nr) % BITS_PER_LONG))
#define BITOP_WORD(nr) ((nr) / BITS_PER_LONG)

static inline int test_bit(int nr, const volatile unsigned long *addr)
{
	return 1UL & (addr[BITOP_WORD(nr)] >> (nr & (BITS_PER_LONG - 1)));
}

#define set_bit(nr, p) ((void) __atomic_fetch_or(&(p)[BITOP_WORD(nr)], BITOP_MASK(nr), __ATOMIC_SEQ_CST))
#define clear_bit(nr, p) ((void) __atomic_fetch_and(&(p)[BITOP_WORD(nr)], ~BITOP_MASK(nr), __ATOMIC_SEQ_CST))
#define change_bit(nr, p) ((void) __atomic_fetch_xor(&(p)[BITOP_WORD(nr)], BITOP_MASK(nr), __ATOMIC_SEQ_CST))

#define test_and_set_bit(nr, p) \
	(!!(__atomic_fetch_or(&(p)[BITOP_WORD(nr)], BITOP_MASK(nr), __ATOMIC_SEQ_CST) & BITOP_MASK(nr)))
#define test_and_clear_bit(nr, p) \
	(!!(__atomic_fetch_and(&(p)[BITOP_WORD(nr)], ~BITOP_MASK(nr), __ATOMIC_SEQ_CST) & BITOP_MASK(nr)))
#define test_and_change_bit(nr, p) \
	(!!(__atomic_fetch_xor(&(p)[BITOP_WORD(nr)], BITOP_MASK(nr), __ATOMIC_SEQ_CST) & BITOP_MASK(nr)))

/* Count leading zeroes */

static inline unsigned long clz(unsigned long word)
{
	return word ? __builtin_clzl(word) : BITS_PER_LONG;
}

/* Position of the least significant 1 (LSB = 0), 0 if none */

static inline unsigned long ffsl(unsigned long word)
{
	return word ? __builtin_ctzl(word) : 0;
}

#endif /*__ASSEMBLY__ */

#endif /* ASM_BITOPS_H */
