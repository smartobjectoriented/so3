/*
 * Copyright (C) 2003 Bernardo Innocenti <bernie@develer.com>
 * Based on former asm-ppc/div64.h and asm-m68knommu/div64.h
 *
 * The semantics of do_div() are:
 *
 * uint32_t do_div(uint64_t *n, uint32_t base)
 * {
 *	uint32_t remainder = *n % base;
 *	*n = *n / base;
 *	return remainder;
 * }
 *
 * NOTE: macro parameter n is evaluated multiple times,
 *       beware of side effects!
 */

#ifndef ASM_GENERIC_DIV64_H
#define ASM_GENERIC_DIV64_H

#include <types.h>

extern uint32_t __div64_32(uint64_t *dividend, uint32_t divisor);

/* _MNR_ TODO do it correctly */
#define do_div(n,base) ({				\
	0xdeadbeef;					\
 })
/* Wrapper for do_div(). Doesn't modify dividend and returns
 * the result, not reminder.
 */
static inline uint64_t lldiv(uint64_t dividend, uint32_t divisor)
{
	uint64_t __res = dividend;
	do_div(__res, divisor);
	return(__res);
}

#endif /* ASM_GENERIC_DIV64_H */
