/*
 * Copyright (C) 2014 Regents of the University of California
 * Copyright (C) 2021 Nicolas Müller <nicolas.muller1@heig-vd.ch>
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

typedef struct { volatile int counter; } atomic_t;
# if 0 /* _NMR_ */
typedef struct { volatile long long counter; } atomic64_t;
#endif

#define ATOMIC_INIT(i)	{ (i) }

#define _atomic_read(v) ((v).counter)
#define atomic_read(v)	((v)->counter)
#define atomic_xchg(v, new) (xchg(&((v)->counter), new))

static inline void atomic_set(atomic_t *v, int i)
{
	(*(volatile int *)(&v->counter) = i);
}

/* Using GCC built-in atomic functions */
#define _priv_atomic_add(ptr, inc) __sync_fetch_and_add(ptr, inc)
#define _priv_atomic_sub(ptr, dec) __sync_fetch_and_sub(ptr, dec)

#define _atomic_set(v,i)	(((v).counter) = (i))

static inline int atomic_cmpxchg(atomic_t *ptr, int old, int new)
{
	return __sync_val_compare_and_swap(&ptr->counter, old, new);
}

/*
 * Atomic compare and exchange.
 */
static inline unsigned long wrong_size_cmpxchg(volatile void *ptr) { BUG(); return 0; }

#define __cmpxchg(ptr, old, new, size)					\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(*(ptr)) __old = (old);				\
	__typeof__(*(ptr)) __new = (new);				\
	__typeof__(*(ptr)) __ret;					\
	register unsigned int __rc;					\
	switch (size) {							\
	case 4:								\
		__asm__ __volatile__ (					\
			"0:	lr.w %0, %2\n"				\
			"	bne  %0, %z3, 1f\n"			\
			"	sc.w.rl %1, %z4, %2\n"			\
			"	bnez %1, 0b\n"				\
			"	fence rw, rw\n"				\
			"1:\n"						\
			: "=&r" (__ret), "=&r" (__rc), "+A" (*__ptr)	\
			: "rJ" ((long)__old), "rJ" (__new)		\
			: "memory");					\
		break;							\
	case 8:								\
		__asm__ __volatile__ (					\
			"0:	lr.d %0, %2\n"				\
			"	bne %0, %z3, 1f\n"			\
			"	sc.d.rl %1, %z4, %2\n"			\
			"	bnez %1, 0b\n"				\
			"	fence rw, rw\n"				\
			"1:\n"						\
			: "=&r" (__ret), "=&r" (__rc), "+A" (*__ptr)	\
			: "rJ" (__old), "rJ" (__new)			\
			: "memory");					\
		break;							\
	default:							\
		wrong_size_cmpxchg(ptr);						\
	}								\
	__ret;								\
})

#define cmpxchg(ptr, o, n)						\
({									\
	__typeof__(*(ptr)) _o_ = (o);					\
	__typeof__(*(ptr)) _n_ = (n);					\
	(__typeof__(*(ptr))) __cmpxchg((ptr),				\
				       _o_, _n_, sizeof(*(ptr)));	\
})

#define __xchg(ptr, new, size)						\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(new) __new = (new);					\
	__typeof__(*(ptr)) __ret;					\
	switch (size) {							\
	case 4:								\
		__asm__ __volatile__ (					\
			"	amoswap.w.aqrl %0, %2, %1\n"		\
			: "=r" (__ret), "+A" (*__ptr)			\
			: "r" (__new)					\
			: "memory");					\
		break;							\
	case 8:								\
		__asm__ __volatile__ (					\
			"	amoswap.d.aqrl %0, %2, %1\n"		\
			: "=r" (__ret), "+A" (*__ptr)			\
			: "r" (__new)					\
			: "memory");					\
		break;							\
	default:							\
		__ret = 0;						\
		printk("!! __bad_xchg called! Failure...\n"); \
		while (1); \
	}								\
	__ret;								\
})

#define xchg(ptr, x)							\
({									\
	__typeof__(*(ptr)) _x_ = (x);					\
	(__typeof__(*(ptr))) __xchg((ptr), _x_, sizeof(*(ptr)));	\
})

#define atomic_xchg(v, new) (xchg(&((v)->counter), new))

static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
	int c, old;

	c = atomic_read(v);
	while (c != u && (old = atomic_cmpxchg((v), c, c + a)) != c)
		c = old;
	return c != u;
}
#define atomic_inc_not_zero(v) atomic_add_unless((v), 1, 0)

#define atomic_add(i, v)	(void) _priv_atomic_add(&(v)->counter, i)
#define atomic_inc(v)		(void) _priv_atomic_add(&(v)->counter, 1)
#define atomic_sub(i, v)	(void) _priv_atomic_sub(&(v)->counter, i)
#define atomic_dec(v)		(void) _priv_atomic_add(&(v)->counter, -1)

#define atomic_inc_and_test(v)	(_priv_atomic_add(&(v)->counter, 1) == 0)
#define atomic_dec_and_test(v)	(_priv_atomic_add(&(v)->counter, -1) == 0)
#define atomic_inc_return(v)    (_priv_atomic_add(&(v)->counter, 1))
#define atomic_dec_return(v)    (_priv_atomic_add(&(v)->counter, -1))
#define atomic_sub_and_test(i, v) (_priv_atomic_sub(&(v)->counter, i) == 0)

#define atomic_add_negative(i,v) (_priv_atomic_add(&(v)->counter, i) < 0)

#if 0 /* _NMR_ */
#include <asm/atomic-generic.h>
#endif

# define atomic_compareandswap(old, new, v)	\
	((atomic_t) { atomic_cmpxchg(v, atomic_read(&old), atomic_read(&new)) })

#endif /* ASM_ATOMIC_H */
