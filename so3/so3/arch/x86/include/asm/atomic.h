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

#ifndef ASM_ATOMIC_H
#define ASM_ATOMIC_H

#include <common.h>
#include <compiler.h>

#include <asm/processor.h>

typedef struct {
	volatile int counter;
} atomic_t;

#define ATOMIC_INIT(i) { (i) }

#define atomic_read(v) READ_ONCE((v)->counter)
#define atomic_set(v, i) WRITE_ONCE(((v)->counter), (i))

static inline int atomic_add_return(int i, atomic_t *v)
{
	return __atomic_add_fetch(&v->counter, i, __ATOMIC_SEQ_CST);
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
	return __atomic_sub_fetch(&v->counter, i, __ATOMIC_SEQ_CST);
}

static inline void atomic_and(int m, atomic_t *v)
{
	__atomic_and_fetch(&v->counter, m, __ATOMIC_SEQ_CST);
}

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
	__atomic_compare_exchange_n(&v->counter, &old, new, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

	return old;
}

static inline int __atomic_add_unless(atomic_t *v, int a, int u)
{
	int c = atomic_read(v);

	while (c != u && !__atomic_compare_exchange_n(&v->counter, &c, c + a, false, __ATOMIC_SEQ_CST,
						      __ATOMIC_SEQ_CST))
		;

	return c;
}

#define atomic_add_unless(v, a, u) (__atomic_add_unless(v, a, u) != (u))
#define atomic_inc_not_zero(v) atomic_add_unless((v), 1, 0)

#define atomic_add(i, v) (void) atomic_add_return(i, v)
#define atomic_inc(v) (void) atomic_add_return(1, v)
#define atomic_sub(i, v) (void) atomic_sub_return(i, v)
#define atomic_dec(v) (void) atomic_sub_return(1, v)

#define atomic_inc_and_test(v) (atomic_add_return(1, v) == 0)
#define atomic_dec_and_test(v) (atomic_sub_return(1, v) == 0)
#define atomic_inc_return(v) (atomic_add_return(1, v))
#define atomic_dec_return(v) (atomic_sub_return(1, v))
#define atomic_sub_and_test(i, v) (atomic_sub_return(i, v) == 0)

#define atomic_add_negative(i, v) (atomic_add_return(i, v) < 0)

#define xchg(ptr, x) __atomic_exchange_n((ptr), (x), __ATOMIC_SEQ_CST)

#define cmpxchg(ptr, o, n)                                                                                 \
	({                                                                                                 \
		__typeof__(*(ptr)) __old = (o);                                                            \
		__atomic_compare_exchange_n((ptr), &__old, (n), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
		__old;                                                                                     \
	})

#define atomic_xchg(v, new) xchg(&((v)->counter), (new))

#endif /* ASM_ATOMIC_H */
