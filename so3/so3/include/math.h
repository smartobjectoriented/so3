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

#endif /* MATH_H */
