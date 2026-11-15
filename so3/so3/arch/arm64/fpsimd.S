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

// The FP/SIMD state of a thread or of a domain: v0 to v31, FPSR and FPCR.
// The layout is struct fpsimd_state of asm/processor.h.

#include <linkage.h>

#include <generated/asm-offsets.h>

// Store the FP/SIMD registers.
// x0: the struct fpsimd_state to fill

ENTRY(fpsimd_save)
	stp	q0, q1, [x0, #(16 * 0)]
	stp	q2, q3, [x0, #(16 * 2)]
	stp	q4, q5, [x0, #(16 * 4)]
	stp	q6, q7, [x0, #(16 * 6)]
	stp	q8, q9, [x0, #(16 * 8)]
	stp	q10, q11, [x0, #(16 * 10)]
	stp	q12, q13, [x0, #(16 * 12)]
	stp	q14, q15, [x0, #(16 * 14)]
	stp	q16, q17, [x0, #(16 * 16)]
	stp	q18, q19, [x0, #(16 * 18)]
	stp	q20, q21, [x0, #(16 * 20)]
	stp	q22, q23, [x0, #(16 * 22)]
	stp	q24, q25, [x0, #(16 * 24)]
	stp	q26, q27, [x0, #(16 * 26)]
	stp	q28, q29, [x0, #(16 * 28)]
	stp	q30, q31, [x0, #(16 * 30)]

	mrs	x1, fpsr
	str	w1, [x0, #OFFSET_FPSIMD_FPSR]
	mrs	x1, fpcr
	str	w1, [x0, #OFFSET_FPSIMD_FPCR]

	ret

// Load the FP/SIMD registers.
// x0: the struct fpsimd_state to load

ENTRY(fpsimd_restore)
	ldp	q0, q1, [x0, #(16 * 0)]
	ldp	q2, q3, [x0, #(16 * 2)]
	ldp	q4, q5, [x0, #(16 * 4)]
	ldp	q6, q7, [x0, #(16 * 6)]
	ldp	q8, q9, [x0, #(16 * 8)]
	ldp	q10, q11, [x0, #(16 * 10)]
	ldp	q12, q13, [x0, #(16 * 12)]
	ldp	q14, q15, [x0, #(16 * 14)]
	ldp	q16, q17, [x0, #(16 * 16)]
	ldp	q18, q19, [x0, #(16 * 18)]
	ldp	q20, q21, [x0, #(16 * 20)]
	ldp	q22, q23, [x0, #(16 * 22)]
	ldp	q24, q25, [x0, #(16 * 24)]
	ldp	q26, q27, [x0, #(16 * 26)]
	ldp	q28, q29, [x0, #(16 * 28)]
	ldp	q30, q31, [x0, #(16 * 30)]

	ldr	w1, [x0, #OFFSET_FPSIMD_FPSR]
	msr	fpsr, x1
	ldr	w1, [x0, #OFFSET_FPSIMD_FPCR]
	msr	fpcr, x1

	ret
