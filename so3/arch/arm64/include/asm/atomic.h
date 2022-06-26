/*
 *  linux/include/asm-arm/atomic.h
 *
 * Copyright (C) 1996 Russell King.
 * Copyright (C) 2002 Deep Blue Solutions Ltd.
 * Copyright (C) 2012 ARM Ltd.
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

#define atomic_read(v)			READ_ONCE((v)->counter)
#define atomic_set(v, i)		WRITE_ONCE(((v)->counter), (i))

/*
 * AArch64 UP and SMP safe atomic ops.  We use load exclusive and
 * store exclusive to ensure that these are atomic.  We may loop
 * to ensure that the update happens.
 */
static inline void atomic_add(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	asm volatile("// atomic_add\n"
"1:	ldxr	%w0, %2\n"
"	add	%w0, %w0, %w3\n"
"	stxr	%w1, %w0, %2\n"
"	cbnz	%w1, 1b"
	: "=&r" (result), "=&r" (tmp), "+Q" (v->counter)
	: "Ir" (i));
}

static inline int atomic_add_return(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	asm volatile("// atomic_add_return\n"
"1:	ldxr	%w0, %2\n"
"	add	%w0, %w0, %w3\n"
"	stlxr	%w1, %w0, %2\n"
"	cbnz	%w1, 1b"
	: "=&r" (result), "=&r" (tmp), "+Q" (v->counter)
	: "Ir" (i)
	: "memory");

	smp_mb();
	return result;
}

static inline void atomic_sub(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	asm volatile("// atomic_sub\n"
"1:	ldxr	%w0, %2\n"
"	sub	%w0, %w0, %w3\n"
"	stxr	%w1, %w0, %2\n"
"	cbnz	%w1, 1b"
	: "=&r" (result), "=&r" (tmp), "+Q" (v->counter)
	: "Ir" (i));
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	asm volatile("// atomic_sub_return\n"
"1:	ldxr	%w0, %2\n"
"	sub	%w0, %w0, %w3\n"
"	stlxr	%w1, %w0, %2\n"
"	cbnz	%w1, 1b"
	: "=&r" (result), "=&r" (tmp), "+Q" (v->counter)
	: "Ir" (i)
	: "memory");

	smp_mb();
	return result;
}

static inline void atomic_and(int m, atomic_t *v)
{
	unsigned long tmp;
	int result;

	asm volatile("// atomic_and\n"
"1:	ldxr	%w0, %2\n"
"	and	%w0, %w0, %w3\n"
"	stxr	%w1, %w0, %2\n"
"	cbnz	%w1, 1b"
	: "=&r" (result), "=&r" (tmp), "+Q" (v->counter)
	: "Ir" (m));
}

static inline int atomic_cmpxchg(atomic_t *ptr, int old, int new)
{
	unsigned long tmp;
	int oldval;

	smp_mb();

	asm volatile("// atomic_cmpxchg\n"
"1:	ldxr	%w1, %2\n"
"	cmp	%w1, %w3\n"
"	b.ne	2f\n"
"	stxr	%w0, %w4, %2\n"
"	cbnz	%w0, 1b\n"
"2:"
	: "=&r" (tmp), "=&r" (oldval), "+Q" (ptr->counter)
	: "Ir" (old), "r" (new)
	: "cc");

	smp_mb();
	return oldval;
}

static inline int __atomic_add_unless(atomic_t *v, int a, int u)
{
	int c, old;

	c = atomic_read(v);
	while (c != u && (old = atomic_cmpxchg((v), c, c + a)) != c)
		c = old;
	return c;
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

/****************************/

static inline unsigned long __xchg(unsigned long x, volatile void *ptr, int size)
{
	unsigned long ret, tmp;

	switch (size) {
	case 1:
		asm volatile("//	__xchg1\n"
		"1:	ldxrb	%w0, %2\n"
		"	stlxrb	%w1, %w3, %2\n"
		"	cbnz	%w1, 1b\n"
			: "=&r" (ret), "=&r" (tmp), "+Q" (*(u8 *)ptr)
			: "r" (x)
			: "memory");
		break;
	case 2:
		asm volatile("//	__xchg2\n"
		"1:	ldxrh	%w0, %2\n"
		"	stlxrh	%w1, %w3, %2\n"
		"	cbnz	%w1, 1b\n"
			: "=&r" (ret), "=&r" (tmp), "+Q" (*(u16 *)ptr)
			: "r" (x)
			: "memory");
		break;
	case 4:
		asm volatile("//	__xchg4\n"
		"1:	ldxr	%w0, %2\n"
		"	stlxr	%w1, %w3, %2\n"
		"	cbnz	%w1, 1b\n"
			: "=&r" (ret), "=&r" (tmp), "+Q" (*(u32 *)ptr)
			: "r" (x)
			: "memory");
		break;
	case 8:
		asm volatile("//	__xchg8\n"
		"1:	ldxr	%0, %2\n"
		"	stlxr	%w1, %3, %2\n"
		"	cbnz	%w1, 1b\n"
			: "=&r" (ret), "=&r" (tmp), "+Q" (*(u64 *)ptr)
			: "r" (x)
			: "memory");
		break;
	default:
		BUG();
		break;
	}

	smp_mb();
	return ret;
}

#define xchg(ptr,x) \
({ \
	__typeof__(*(ptr)) __ret; \
	__ret = (__typeof__(*(ptr))) \
		__xchg((unsigned long)(x), (ptr), sizeof(*(ptr))); \
	__ret; \
})

extern unsigned long __bad_cmpxchg(volatile void *ptr, int size);

#define __CMPXCHG_CASE(w, sz, name)					\
static inline bool __cmpxchg_case_##name(volatile void *ptr,		\
					 unsigned long *old,		\
					 unsigned long new,		\
					 bool timeout,			\
					 unsigned int max_try)		\
{									\
	unsigned long oldval;						\
	unsigned long res;						\
									\
	do {								\
		asm volatile("// __cmpxchg_case_" #name "\n"		\
		"	ldxr" #sz "	%" #w "1, %2\n"			\
		"	mov	%w0, #0\n"				\
		"	cmp	%" #w "1, %" #w "3\n"			\
		"	b.ne	1f\n"					\
		"	stxr" #sz "	%w0, %" #w "4, %2\n"		\
		"1:\n"							\
		: "=&r" (res), "=&r" (oldval),				\
		  "+Q" (*(unsigned long *)ptr)				\
		: "Ir" (*old), "r" (new)				\
		: "cc");						\
									\
		if (!res)						\
			break;						\
	} while (!timeout || ((--max_try) > 0));			\
									\
	*old = oldval;							\
									\
	return !res;							\
}

__CMPXCHG_CASE(w, b, 1)
__CMPXCHG_CASE(w, h, 2)
__CMPXCHG_CASE(w,  , 4)
__CMPXCHG_CASE( ,  , 8)

static inline bool __int_cmpxchg(volatile void *ptr, unsigned long *old,
					unsigned long new, int size,
					bool timeout, unsigned int max_try)
{
	switch (size) {
	case 1:
		return __cmpxchg_case_1(ptr, old, new, timeout, max_try);
	case 2:
		return __cmpxchg_case_2(ptr, old, new, timeout, max_try);
	case 4:
		return __cmpxchg_case_4(ptr, old, new, timeout, max_try);
	case 8:
		return __cmpxchg_case_8(ptr, old, new, timeout, max_try);
	default:
		BUG();
	}

	return false;
}

static inline unsigned long __cmpxchg(volatile void *ptr,
					     unsigned long old,
					     unsigned long new,
					     int size)
{
	smp_mb();
	if (!__int_cmpxchg(ptr, &old, new, size, false, 0))
		BUG();
	smp_mb();

	return old;
}

/*
 * The helper may fail to update the memory if the action takes too long.
 *
 * @old: On call the value pointed contains the expected old value. It will be
 * updated to the actual old value.
 * @max_try: Maximum number of iterations
 *
 * The helper will return true when the update has succeeded (i.e no
 * timeout) and false if the update has failed.
 */
static always_inline bool __cmpxchg_timeout(volatile void *ptr,
					    unsigned long *old,
					    unsigned long new,
					    int size,
					    unsigned int max_try)
{
	bool ret;

	smp_mb();
	ret = __int_cmpxchg(ptr, old, new, size, true, max_try);
	smp_mb();

	return ret;
}

#define cmpxchg(ptr, o, n) \
({ \
	__typeof__(*(ptr)) __ret; \
	__ret = (__typeof__(*(ptr))) \
		__cmpxchg((ptr), (unsigned long)(o), (unsigned long)(n), \
			  sizeof(*(ptr))); \
	__ret; \
})

#define atomic_xchg(v, new)		xchg(&((v)->counter), (new))


#endif /* ASM_ATOMIC_H */
