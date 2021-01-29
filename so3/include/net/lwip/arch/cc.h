/*
 * Copyright (C) 2020 Julien Quartier <julien.quartier@heig-vd.ch>
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


#ifndef CC_H
#define CC_H

#ifndef LWIP_PLATFORM_DIAG
#define LWIP_PLATFORM_DIAG(x) do {printk x;} while(0)
#include <printk.h>
#endif

#ifndef LWIP_PLATFORM_ASSERT
#define LWIP_PLATFORM_ASSERT(x) do {printk("Assertion \"%s\" failed at line %d in %s\n", \
                                         x, __LINE__, __FILE__); fflush(NULL); abort();} while(0)

#include <printk.h>
#endif


/**
 *
 */
#include <errno.h>
#define set_errno(err) set_errno(err)


#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS

#define LWIP_NO_STDINT_H 1
#define LWIP_NO_INTTYPES_H 1

// Required functions defined in compiler.c
#define LWIP_NO_STDDEF_H 1


#define X8_F  "02x"
#define U16_F "d"
#define S16_F "d"
#define X16_F "x"
#define U32_F "d"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "lu"

#include <types.h>
typedef u8      u8_t;
typedef signed char s8_t;
typedef u16     u16_t;
typedef s16     s16_t;
typedef u32     u32_t;
typedef s32     s32_t;

typedef unsigned long int  mem_ptr_t;

#include <timer.h>
#define LWIP_TIMEVAL_PRIVATE 0

#endif /* CC_H */
