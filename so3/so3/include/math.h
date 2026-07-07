/*
 * Copyright (c) 2017-2026 REDS Institute, HEIG-VD
 * Author: Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef MATH_H
#define MATH_H

#include <types.h>

#ifndef __ASSEMBLY__

#define max(a, b)                  \
	({                         \
		typeof(a) _a = a;  \
		typeof(b) _b = b;  \
		_a > _b ? _a : _b; \
	})

#define min(a, b)                  \
	({                         \
		typeof(a) _a = a;  \
		typeof(b) _b = b;  \
		_a < _b ? _a : _b; \
	})

/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max at all, of course.
 */
#define min_t(type, x, y)              \
	({                             \
		type __x = (x);        \
		type __y = (y);        \
		__x < __y ? __x : __y; \
	})
#define max_t(type, x, y)              \
	({                             \
		type __x = (x);        \
		type __y = (y);        \
		__x > __y ? __x : __y; \
	})

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define DIV_ROUND_CLOSEST(x, divisor)                                                                                 \
	({                                                                                                            \
		typeof(x) __x = x;                                                                                    \
		typeof(divisor) __d = divisor;                                                                        \
		(((typeof(x)) -1) > 0 || ((typeof(divisor)) -1) > 0 || (__x) > 0) ? (((__x) + ((__d) / 2)) / (__d)) : \
										    (((__x) - ((__d) / 2)) / (__d));  \
	})

/*
 * This looks more complex than it should be. But we need to
 * get the type for the ~ right in round_down (it needs to be
 * as wide as the result!), and we want to evaluate the macro
 * arguments just once each.
 */
#define __round_mask(x, y) ((__typeof__(x)) ((y) - 1))
/**
 * round_up - round up to next specified power of 2
 * @x: the value to round
 * @y: multiple to round up to (must be a power of 2)
 *
 * Rounds @x up to next multiple of @y (which must be a power of 2).
 * To perform arbitrary rounding up, use roundup() below.
 */
#define round_up(x, y) ((((x) - 1) | __round_mask(x, y)) + 1)
/**
 * round_down - round down to next specified power of 2
 * @x: the value to round
 * @y: multiple to round down to (must be a power of 2)
 *
 * Rounds @x down to next multiple of @y (which must be a power of 2).
 * To perform arbitrary rounding down, use rounddown() below.
 */
#define round_down(x, y) ((x) & ~__round_mask(x, y))

/*
 * Round @n up to the next power of two. Returns @n unchanged when it already is
 * a power of two, and 1 for @n == 0.
 */
static inline size_t round_up_pow2(size_t n)
{
	size_t p = 1;

	while (p < n)
		p <<= 1;

	return p;
}

/*
 * Integer base-2 logarithm of @n, rounded up to the next integer. Originally
 * from U-Boot -- Copyright (C) 2010 Texas Instruments, Aneesh V <aneesh@ti.com>.
 */
static inline s32 log_2_n_round_up(u32 n)
{
	s32 log2n = -1;
	u32 temp = n;

	while (temp) {
		log2n++;
		temp >>= 1;
	}

	if (n & (n - 1))
		return log2n + 1; /* not power of 2 - round up */
	else
		return log2n; /* power of 2 */
}

/* Integer base-2 logarithm of @n, rounded down (i.e. the index of the MSB). */
static inline s32 log_2_n_round_down(u32 n)
{
	s32 log2n = -1;
	u32 temp = n;

	while (temp) {
		log2n++;
		temp >>= 1;
	}

	return log2n;
}

#endif /* __ASSEMBLY__ */

#endif /* MATH_H */
