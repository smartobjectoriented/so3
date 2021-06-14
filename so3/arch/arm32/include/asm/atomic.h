/*
 *  linux/include/asm-arm/atomic.h
 *
 *  Copyright (C) 1996 Russell King.
 *  Copyright (C) 2002 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef ASM_ATOMIC_H
#define ASM_ATOMIC_H

#include <common.h>
#include <compiler.h>

#include <asm/processor.h>

typedef struct { volatile int counter; } atomic_t;

#define ATOMIC_INIT(i)	{ (i) }

#define _atomic_read(v) ((v).counter)
#define atomic_read(v)	((v)->counter)

/*
 * ARMv6 UP and SMP safe atomic ops.  We use load exclusive and
 * store exclusive to ensure that these are atomic.  We may loop
 * to ensure that the update happens.  Writing to 'v->counter'
 * without using the following operations WILL break the atomic
 * nature of these ops.
 */
static inline void atomic_set(atomic_t *v, int i)
{
	unsigned long tmp;

	__asm__ __volatile__("@ atomic_set\n"
"1:	ldrex	%0, [%1]\n"
"	strex	%0, %2, [%1]\n"
"	teq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (&v->counter), "r" (i)
	: "cc");
}
#define _atomic_set(v,i)	(((v).counter) = (i))
static inline int atomic_add_return(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	__asm__ __volatile__("@ atomic_add_return\n"
"1:	ldrex	%0, [%2]\n"
"	add	%0, %0, %3\n"
"	strex	%1, %0, [%2]\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp)
	: "r" (&v->counter), "Ir" (i)
	: "cc");

	return result;
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	__asm__ __volatile__("@ atomic_sub_return\n"
"1:	ldrex	%0, [%2]\n"
"	sub	%0, %0, %3\n"
"	strex	%1, %0, [%2]\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp)
	: "r" (&v->counter), "Ir" (i)
	: "cc");

	return result;
}

static inline int atomic_cmpxchg(atomic_t *ptr, int old, int new)
{
	unsigned long oldval, res;

	do {
		__asm__ __volatile__("@ atomic_cmpxchg\n"
		"ldrex	%1, [%2]\n"
		"mov	%0, #0\n"
		"teq	%1, %3\n"
		"strexeq %0, %4, [%2]\n"
		    : "=&r" (res), "=&r" (oldval)
		    : "r" (&ptr->counter), "Ir" (old), "r" (new)
		    : "cc");
	} while (res);

	return oldval;
}

static inline void atomic_clear_mask(unsigned long mask, unsigned long *addr)
{
	unsigned long tmp, tmp2;

	__asm__ __volatile__("@ atomic_clear_mask\n"
"1:	ldrex	%0, [%2]\n"
"	bic	%0, %0, %3\n"
"	strex	%1, %0, [%2]\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (tmp), "=&r" (tmp2)
	: "r" (addr), "Ir" (mask)
	: "cc");
}

/*
 * Atomic compare and exchange.
 */

static inline unsigned long wrong_size_cmpxchg(volatile void *ptr) { BUG(); return 0; }

static inline unsigned long __cmpxchg(volatile void *ptr,
				unsigned long old,
				unsigned long new, int size)
{
	volatile unsigned long *p = ptr;

	if (size == 4) {
		unsigned long oldval, res;

		do {
			__asm__ __volatile__("@ atomic_cmpxchg\n"
			"ldrex  %1, [%2]\n"
			"mov    %0, #0\n"
			"teq    %1, %3\n"
			"strexeq %0, %4, [%2]\n"
			: "=&r" (res), "=&r" (oldval)
			: "r" (p), "Ir" (old), "r" (new)
			: "cc");
		} while (res);

		return oldval;
	} else
		return wrong_size_cmpxchg(ptr);
}

#define cmpxchg(ptr,o,n)						\
({									\
	__typeof__(*(ptr)) _o_ = (o);					\
	__typeof__(*(ptr)) _n_ = (n);					\
	(__typeof__(*(ptr))) __cmpxchg((ptr), (unsigned long)_o_,	\
		 (unsigned long)_n_, sizeof(*(ptr)));			\
})

static void __bad_xchg(volatile void *ptr, int size) {
	printk("!! __bad_xchg called! Failure...\n");
	while (1);
}

static inline unsigned long __xchg(unsigned long x, volatile void *ptr, int size)
{
	unsigned long ret;
	unsigned int tmp;

	switch (size) {

	case 1:
		asm volatile("@	__xchg1\n"
		"1:	ldrexb	%0, [%3]\n"
		"	strexb	%1, %2, [%3]\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;

case 4:
		asm volatile("@	__xchg4\n"
		"1:	ldrex	%0, [%3]\n"
		"	strex	%1, %2, [%3]\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;

	default:
		__bad_xchg(ptr, size), ret = 0;
		break;
	}

	return ret;
}

#define xchg(ptr,x) \
	((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))


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

#define atomic_add(i, v)	(void) atomic_add_return(i, v)
#define atomic_inc(v)		(void) atomic_add_return(1, v)
#define atomic_sub(i, v)	(void) atomic_sub_return(i, v)
#define atomic_dec(v)		(void) atomic_sub_return(1, v)

#define atomic_inc_and_test(v)	(atomic_add_return(1, v) == 0)
#define atomic_dec_and_test(v)	(atomic_sub_return(1, v) == 0)
#define atomic_inc_return(v)    (atomic_add_return(1, v))
#define atomic_dec_return(v)    (atomic_sub_return(1, v))
#define atomic_sub_and_test(i, v) (atomic_sub_return(i, v) == 0)

#define atomic_add_negative(i,v) (atomic_add_return(i, v) < 0)

#include <asm/atomic-generic.h>

# define atomic_compareandswap(old, new, v)	\
	((atomic_t) { atomic_cmpxchg(v, atomic_read(&old), atomic_read(&new)) })


#endif /* ASM_ATOMIC_H */
