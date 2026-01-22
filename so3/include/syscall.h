/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017 Xavier Ruppen <xavier.ruppen@heig-vd.ch>
 * Copyright (C) 2017 Alexandre Malki <alexandre.malki@heig-vd.ch>
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

#ifndef ASM_ARM_SYSCALL_H
#define ASM_ARM_SYSCALL_H

#include <generated/syscall_number.h>

#ifndef __ASSEMBLY__

#include <errno.h>
#include <types.h>

#define NR_SYSCALLS 468

/**
 * Map arguments set (type, name) into a comma separated list
 * e.g. function arguments
 *
 * @param x Number of arguments to map
 * @param m Macro mapping type and name correctly
 * @param t Type of the function arguments
 * @param n name of the function arguments
 */
#define __MAP1(m, t, n) m(t, n)
#define __MAP2(m, t, n, ...) m(t, n), __MAP1(m, __VA_ARGS__)
#define __MAP3(m, t, n, ...) m(t, n), __MAP2(m, __VA_ARGS__)
#define __MAP4(m, t, n, ...) m(t, n), __MAP3(m, __VA_ARGS__)
#define __MAP5(m, t, n, ...) m(t, n), __MAP4(m, __VA_ARGS__)
#define __MAP6(m, t, n, ...) m(t, n), __MAP5(m, __VA_ARGS__)
#define __MAP(x, m, ...) __MAP##x(m, __VA_ARGS__)

/** Macros to map type and name with __MAP */
#define __M_DECL(t, n) t n
#define __M_LONG(t, n) long n
#define __M_ARGS(t, n) n

/**
 * Map syscall arguments by casting them for function call.
 *
 * @param x Number of arguments to map
 * @param args Arguments array to use
 * @param m Macro mapping type and name correctly
 * @param t Type of the function arguments
 * @param n name of the function arguments, not use but allow
 *          to use same __VA_ARGS__ as in __MAP()
 */
#define __MAP_ARGS1(x, args, t, n) (t) args[x - 1]
#define __MAP_ARGS2(x, args, t, n, ...) (t) args[x - 2], __MAP_ARGS1(x, args, __VA_ARGS__)
#define __MAP_ARGS3(x, args, t, n, ...) (t) args[x - 3], __MAP_ARGS2(x, args, __VA_ARGS__)
#define __MAP_ARGS4(x, args, t, n, ...) (t) args[x - 4], __MAP_ARGS3(x, args, __VA_ARGS__)
#define __MAP_ARGS5(x, args, t, n, ...) (t) args[x - 5], __MAP_ARGS4(x, args, __VA_ARGS__)
#define __MAP_ARGS6(x, args, t, n, ...) (t) args[x - 6], __MAP_ARGS5(x, args, __VA_ARGS__)
#define __MAP_ARGS(x, args, ...) __MAP_ARGS##x((x), (args), __VA_ARGS__)

/**
 * Syscall functions declaration and definitions helper.
 *
 * Two functions are added for each syscall.
 *
 * __sys_SYSCALL_NAME taking syscall_args_t as arguments allowing for
 * a common interface between all syscall to put them in an array.
 *
 * sys_do_SYSCALL_NAME actual function with the syscall implementation with
 * arguments define like a normal function. That function is automatically
 * called by __sys_SYCALL_NAME with the correct argument number and cast.
 */
#define SYSCALL_DECLARE(name, ...)               \
	long __sys_##name(syscall_args_t *args); \
	inline long sys_do_##name(__VA_ARGS__);

#define SYSCALL_DEFINE0(name)                     \
	long __sys_##name(syscall_args_t *unused) \
	{                                         \
		return sys_do_##name();           \
	}                                         \
	inline long sys_do_##name(void)
#define SYSCALL_DEFINE1(...) __SYSCALL_DEFINEx(1, __VA_ARGS__)
#define SYSCALL_DEFINE2(...) __SYSCALL_DEFINEx(2, __VA_ARGS__)
#define SYSCALL_DEFINE3(...) __SYSCALL_DEFINEx(3, __VA_ARGS__)
#define SYSCALL_DEFINE4(...) __SYSCALL_DEFINEx(4, __VA_ARGS__)
#define SYSCALL_DEFINE5(...) __SYSCALL_DEFINEx(5, __VA_ARGS__)
#define SYSCALL_DEFINE6(...) __SYSCALL_DEFINEx(6, __VA_ARGS__)

#define __SYSCALL_DEFINEx(x, name, ...)                                       \
	long __sys_##name(syscall_args_t *args)                               \
	{                                                                     \
		return sys_do_##name(__MAP_ARGS(x, args->args, __VA_ARGS__)); \
	}                                                                     \
	inline long sys_do_##name(__MAP(x, __M_DECL, __VA_ARGS__))

typedef struct {
	unsigned long args[6];
} syscall_args_t;

typedef long (*syscall_fn_t)(syscall_args_t *);

long syscall_handle(syscall_args_t *);

#endif /* __ASSEMBLY__ */

#endif /* ASM_ARM_SYSCALL_H */
