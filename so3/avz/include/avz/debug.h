/*
 * Copyright (C) 2015-2019 Daniel Rossier <daniel.rossier@soo.tech>
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

#ifndef UAPI_DEBUG_H
#define UAPI_DEBUG_H

#ifdef __KERNEL__
#define force_print lprintk
#else
#define force_print printf
#endif /* __KERNEL__ */

#ifdef DEBUG
#undef DBG

#ifdef __KERNEL__

#include <avz/console.h>

#endif /* __KERNEL__ */

/*
 * force_print() is able to manage if the function is called from the kernel or the user space
*/
#define DBG(fmt, ...) \
	do { \
		force_print("%s:%i > "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while (0)

/* rtdm_printk() is broken for the time being. Use lprintk() instead.
 * One can safely switch back to rtdm_printk() once the issues are solved. */
#define RTDBG(fmt, ...) \
	do { \
		force_print("%s:%i > "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while (0)

#define DBG0(...) DBG("%s", ##__VA_ARGS__)

#define DBG_BUFFER(buffer, ...) \
	do { \
		lprintk_buffer(buffer, ##__VA_ARGS__); \
	} while (0)

#else

#define DBG(fmt, ...)
#define RTDBG(fmt, ...)
#define DBG0(...)
#define DBG_BUFFER(buffer, ...)
#define DBG_ON__
#define DBG_OFF__

#endif

#endif /* UAPI_DEBUG_H */

