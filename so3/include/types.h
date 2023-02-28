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

#ifndef TYPES_H
#define TYPES_H

#include <asm/types.h>

#include <compiler.h>

#define BITS_TO_LONGS(bits) \
    (((bits)+BITS_PER_LONG-1)/BITS_PER_LONG)
#define DECLARE_BITMAP(name,bits) \
    unsigned long name[BITS_TO_LONGS(bits)]

/* sizeof() for a structure/union field */
#define FIELD_SIZEOF(type, fld)	(sizeof(((type *)0)->fld))

/* create 64-bit mask with bytes 0 to size-1 set to 0xff */
#define BYTE_MASK(size)		(0xffffffffffffffffULL >> ((8 - (size)) * 8))

/* create 64-bit mask with all bits in [last:first] set */
#define BIT_MASK(last, first) \
	((0xffffffffffffffffULL >> (64 - ((last) + 1 - (first)))) << (first))

/* extract the field value at [last:first] from an input of up to 64 bits */
#define GET_FIELD(value, last, first) \
	(((value) & BIT_MASK((last), (first))) >> (first))

/* set the field value at [last:first] from an input of up to 64 bits*/
#define SET_FIELD(value, last, first) \
	((value) << (first) & BIT_MASK((last), (first)))

/* mandatory macros */
#undef NULL
#define NULL ((void *) 0)

/* mandatory types */

typedef unsigned long size_t;
typedef int ssize_t;

typedef signed int ptrdiff_t;

typedef unsigned int wchar_t;

#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))
#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)

/* Old compatibility names for C types.  */
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;

typedef unsigned char		uchar;
typedef volatile unsigned long	vu_long;
typedef volatile unsigned short vu_short;
typedef volatile unsigned char	vu_char;


typedef		__u8		uint8_t;
typedef		__u8		u8;

typedef		__u16		uint16_t;
typedef		__u16		u16;

typedef		__s16		int16_t;
typedef		__s16		s16;

typedef		__s32		int32_t;
typedef		__s32		s32;

typedef		__u32		uint32_t;
typedef		__u32		u32;

typedef		__u64		uint64_t;
typedef		__u64		u64;

typedef		__s64		int64_t;
typedef		__s64		s64;


/*
 * Below are truly Linux-specific types that should never collide with
 * any application/library that wants linux/types.h.
 */
#ifdef __CHECKER__
#define __bitwise__ __attribute__((bitwise))
#else
#define __bitwise__
#endif
#ifdef __CHECK_ENDIAN__
#define __bitwise __bitwise__
#else
#define __bitwise
#endif

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
#if defined(__GNUC__)
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;
#endif
typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;


typedef unsigned __bitwise__	gfp_t;

typedef u64 time_t;

#define ROUND(a, b)     (((a) + (b) - 1) & ~((b) - 1))

/*
 * The ALLOC_CACHE_ALIGN_BUFFER macro is used to allocate a buffer on the
 * stack that meets the minimum architecture alignment requirements for DMA.
 * Such a buffer is useful for DMA operations where flushing and invalidating
 * the cache before and after a read and/or write operation is required for
 * correct operations.
 *
 * When called the macro creates an array on the stack that is sized such
 * that:
 *
 * 1) The beginning of the array can be advanced enough to be aligned.
 *
 * 2) The size of the aligned portion of the array is a multiple of the minimum
 *    architecture alignment required for DMA.
 *
 * 3) The aligned portion contains enough space for the original number of
 *    elements requested.
 *
 * The macro then creates a pointer to the aligned portion of this array and
 * assigns to the pointer the address of the first element in the aligned
 * portion of the array.
 *
 * Calling the macro as:
 *
 *     ALLOC_CACHE_ALIGN_BUFFER(uint32_t, buffer, 1024);
 *
 * Will result in something similar to saying:
 *
 *     uint32_t    buffer[1024];
 *
 * The following differences exist:
 *
 * 1) The resulting buffer is guaranteed to be aligned to the value of
 *    ARCH_DMA_MINALIGN.
 *
 * 2) The buffer variable created by the macro is a pointer to the specified
 *    type, and NOT an array of the specified type.  This can be very important
 *    if you want the address of the buffer, which you probably do, to pass it
 *    to the DMA hardware.  The value of &buffer is different in the two cases.
 *    In the macro case it will be the address of the pointer, not the address
 *    of the space reserved for the buffer.  However, in the second case it
 *    would be the address of the buffer.  So if you are replacing hard coded
 *    stack buffers with this macro you need to make sure you remove the & from
 *    the locations where you are taking the address of the buffer.
 *
 * Note that the size parameter is the number of array elements to allocate,
 * not the number of bytes.
 *
 * This macro can not be used outside of function scope, or for the creation
 * of a function scoped static buffer.  It can not be used to create a cache
 * line aligned global buffer.
 */
#define PAD_COUNT(s, pad) (((s) - 1) / (pad) + 1)
#define PAD_SIZE(s, pad) (PAD_COUNT(s, pad) * pad)
#define ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, pad)		\
	char __##name[ROUND(PAD_SIZE((size) * sizeof(type), pad), align)  \
		      + (align - 1)];					\
									\
	type *name = (type *) ALIGN((uintptr_t)__##name, align)
#define ALLOC_ALIGN_BUFFER(type, name, size, align)		\
	ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, 1)
#define ALLOC_CACHE_ALIGN_BUFFER_PAD(type, name, size, pad)		\
	ALLOC_ALIGN_BUFFER_PAD(type, name, size, ARCH_DMA_MINALIGN, pad)
#define ALLOC_CACHE_ALIGN_BUFFER(type, name, size)			\
	ALLOC_ALIGN_BUFFER(type, name, size, ARCH_DMA_MINALIGN)

/*
 * DEFINE_CACHE_ALIGN_BUFFER() is similar to ALLOC_CACHE_ALIGN_BUFFER, but it's
 * purpose is to allow allocating aligned buffers outside of function scope.
 * Usage of this macro shall be avoided or used with extreme care!
 */
#define DEFINE_ALIGN_BUFFER(type, name, size, align)			\
	static char __##name[roundup(size * sizeof(type), align)]	\
			__aligned(align);				\
									\
	static type *name = (type *)__##name
#define DEFINE_CACHE_ALIGN_BUFFER(type, name, size)			\
	DEFINE_ALIGN_BUFFER(type, name, size, ARCH_DMA_MINALIGN)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef unsigned char bool;
#define false 	0
#define true	1

/* align addr on a size boundary - adjust address up/down if needed */
#define ALIGN_UP(addr,size) (((addr)+((size)-1))&(~((size)-1)))
#define ALIGN_DOWN(addr,size) ((addr)&(~((size)-1)))

typedef unsigned long addr_t;

/*
 * Fast implementation of tolower() for internal usage. Do not use in your
 * code.
 */
static inline char _tolower(const char c)
{
	return c | 0x20;
}

#define test_and_set_bool(b)   xchg(&(b), 1)
#define test_and_clear_bool(b) xchg(&(b), 0)

#endif /* TYPES_H */
