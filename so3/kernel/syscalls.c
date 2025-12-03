/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017 Alexandre Malki <alexandre.malki@heig-vd.ch>
 * Copyright (C) 2017 Xavier Ruppen <xavier.ruppen@heig-vd.ch>
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

#include <common.h>
#include <process.h>
#include <thread.h>
#include <vfs.h>
#include <pipe.h>
#include <heap.h>
#include <process.h>
#include <signal.h>
#include <timer.h>
#include <net.h>
#include <futex.h>
#include <syscall.h>

extern void __get_syscall_args_ext(long *syscall_no);

extern void test_malloc(int test_no);

static const syscall_fn_t syscall_table[NR_SYSCALLS] = {
	[0 ... NR_SYSCALLS - 1] = NULL,
/* Generated file with table element matching number to functions. */
#include <generated/syscall_table.h.in>
};

/*
 * Process syscalls according to the syscall number passed in r7 on ARM and x8 on ARM64.
 * According to SO3 ABI, the syscall arguments are passed in r0-r5 on ARM and x0-x5 on ARM64.
 */

long syscall_handle(syscall_args_t *syscall_args)
{
	long syscall_no;

	/* Get addtional args of the syscall according to the ARM & SO3 ABI */
	__get_syscall_args_ext(&syscall_no);

	if ((syscall_no >= NR_SYSCALLS) || (syscall_table[syscall_no] == NULL)) {
		printk("%s: unhandled syscall: %d\n", __func__, syscall_no);
		return -ENOSYS;
	}

#warning do_softirq?

	return syscall_table[syscall_no](syscall_args);
}
