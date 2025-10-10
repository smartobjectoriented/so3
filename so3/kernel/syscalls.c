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
#include <syscall.h>

extern void __get_syscall_args_ext(uint32_t *syscall_no);
extern uint32_t __get_syscall_stack_arg(uint32_t nr);

extern void test_malloc(int test_no);

#warning Not updated, a rework is needed to avoid having a big array because of #ifdef ...
static const syscall_fn_t syscall_table[NR_SYSCALLS] = {
	[0 ... NR_SYSCALLS - 1] = NULL,
	/*
#ifdef CONFIG_MMU
	[SYSCALL_GETPID] = __sys_getpid,
#ifdef SYSCALL_GETTIMEOFDAY
	[SYSCALL_GETTIMEOFDAY] = __sys_gettimeofday,
#endif
#ifdef SYSCALL_GETTIMEOFDAY_TIME32
	[SYSCALL_GETTIMEOFDAY_TIME32] = __sys_gettimeofday_time32,
#endif
#ifdef SYSCALL_CLOCK_GETTIME
	[SYSCALL_CLOCK_GETTIME] = __sys_clock_gettime,
#endif
#ifdef SYSCALL_CLOCK_GETTIME32
	[SYSCALL_CLOCK_GETTIME32] = __sys_clock_gettime32,
#endif
	[SYSCALL_EXIT] = __sys_exit,
	[SYSCALL_EXECVE] = __sys_execve,
	[SYSCALL_FORK] = __sys_fork,
#ifdef SYSCALL_WAITPID
	[SYSCALL_WAITPID] = __sys_waitpid,
#endif
	[SYSCALL_WAIT4] = __sys_wait4,
	[SYSCALL_PTRACE] = __sys_ptrace,
#endif
	[SYSCALL_READ] = __sys_read,
	[SYSCALL_WRITE] = __sys_write,
#ifdef SYSCALL_OPEN
	[SYSCALL_OPEN] = __sys_open,
#endif
	[SYSCALL_OPENAT] = __sys_openat,
	[SYSCALL_CLOSE] = __sys_close,
	[SYSCALL_THREAD_CREATE] = __sys_thread_create,
	[SYSCALL_THREAD_JOIN] = __sys_thread_join,
	[SYSCALL_THREAD_EXIT] = __sys_thread_exit,
	[SYSCALL_THREAD_YIELD] = __sys_thread_yield,
	[SYSCALL_GETDENTS64] = __sys_getdents64,
	[SYSCALL_IOCTL] = __sys_ioctl,
	[SYSCALL_LSEEK] = __sys_lseek,
	[SYSCALL_READV] = __sys_readv,
	[SYSCALL_WRITEV] = __sys_writev,
#ifdef CONFIG_IPC_PIPE
#ifdef SYSCALL_PIPE
	[SYSCALL_PIPE] = __sys_pipe,
#endif
	[SYSCALL_PIPE2] = __sys_pipe2,
#endif
	[SYSCALL_DUP] = __sys_dup,
#ifdef SYSCALL_DUP2
	[SYSCALL_DUP2] = __sys_dup2,
#endif
	[SYSCALL_DUP3] = __sys_dup3,
#ifdef SYSCALL_STAT
	[SYSCALL_STAT] = __sys_stat,
#endif
	[SYSCALL_FSTATAT] = __sys_fstatat,
#ifdef SYSCALL_MMAP
	[SYSCALL_MMAP] = __sys_mmap,
#endif
#ifdef SYSCALL_MMAP2
	[SYSCALL_MMAP2] = __sys_mmap2,
#endif
	[SYSCALL_NANOSLEEP] = __sys_nanosleep,
#ifdef CONFIG_PROC_ENV
	[SYSCALL_BRK] = __sys_brk,
#endif
	[SYSCALL_MUTEX_LOCK] = __sys_mutex_lock,
	[SYSCALL_MUTEX_UNLOCK] = __sys_mutex_unlock,
#ifdef CONFIG_IPC_SIGNAL
#ifdef SYSCALL_SIGACTION
	[SYSCALL_SIGACTION] = __sys_sigaction,
#endif
	[SYSCALL_RT_SIGACTION] = __sys_rt_sigaction,
	[SYSCALL_KILL] = __sys_kill,
#ifdef SYSCALL_SIGRETURN
	[SYSCALL_SIGRETURN] = __sys_sigreturn,
#endif
	[SYSCALL_RT_SIGRETURN] = __sys_rt_sigreturn,
#endif
#ifdef CONFIG_NET
	[SYSCALL_SOCKET] = __sys_socket,
	[SYSCALL_BIND] = __sys_bind,
	[SYSCALL_LISTEN] = __sys_listen,
	[SYSCALL_ACCEPT] = __sys_accept,
	[SYSCALL_CONNECT] = __sys_connect,
#ifdef SYSCALL_RECV
	[SYSCALL_RECV] = __sys_recv,
#endif
#ifdef SYSCALL_SEND
	[SYSCALL_SEND] = __sys_send,
#endif
	[SYSCALL_SENDTO] = __sys_sendto,
	[SYSCALL_SETSOCKOPT] = __sys_setsockopt,
	[SYSCALL_RECVFROM] = __sys_recvfrom,
#endif
*/
};

/*
 * Process syscalls according to the syscall number passed in r7 on ARM and x8 on ARM64.
 * According to SO3 ABI, the syscall arguments are passed in r0-r5 on ARM and x0-x5 on ARM64.
 */

long syscall_handle(syscall_args_t *syscall_args)
{
	long result = -1;
	uint32_t syscall_no;

	/* Get addtional args of the syscall according to the ARM & SO3 ABI */
	__get_syscall_args_ext(&syscall_no);

	if ((syscall_no >= NR_SYSCALLS) || (syscall_table[syscall_no] == NULL)) {
		printk("%s: unhandled syscall: %d\n", __func__, syscall_no);
		return -ENOSYS;
	} else {
		return syscall_table[syscall_no](syscall_args);
	}

#warning do_softirq?

	return result;
}
