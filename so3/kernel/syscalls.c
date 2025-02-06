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

static uint32_t *errno_addr = NULL;

extern void __get_syscall_args_ext(uint32_t *syscall_no,
				   uint32_t **__errno_addr);
extern uint32_t __get_syscall_stack_arg(uint32_t nr);

extern void test_malloc(int test_no);

/*
 * Set the (user space) virtual address of the global <errno> variable which is defined in the libc.
 * <errno> is used to keep a error number in case of syscall execution failure.
 */
void set_errno_addr(uint32_t *addr)
{
	errno_addr = addr;
}

uint32_t *get_errno_addr(void)
{
	return errno_addr;
}

/*
 * Set the errno to a specific value defined in errno.h
 */
void set_errno(uint32_t val)
{
	if (errno_addr != NULL)
		*errno_addr = val;
}

/*
 * Process syscalls according to the syscall number passed in r7 on ARM and x8 on ARM64.
 * According to SO3 ABI, the syscall arguments are passed in r0-r5 on ARM and x0-x5 on ARM64.
 */

long syscall_handle(syscall_args_t *a)
{
	long result = -1;
	uint32_t syscall_no, *__errno_addr;

	/* Get addtional args of the syscall according to the ARM & SO3 ABI */
	__get_syscall_args_ext(&syscall_no, &__errno_addr);

	set_errno_addr(__errno_addr);

	switch (syscall_no) {
#ifdef CONFIG_MMU
	case SYSCALL_GETPID:
		result = do_getpid();
		break;

	case SYSCALL_GETTIMEOFDAY:
		/* a->args[1] contains a pointer to the timezone structure. */
		/* Currently, this is not supported yet. */

		result = do_get_time_of_day((struct timespec *)a->args[0]);
		break;

	case SYSCALL_CLOCK_GETTIME:

		result = do_get_clock_time(a->args[0],
					   (struct timespec *)a->args[1]);
		break;

	case SYSCALL_SETTIMEOFDAY:

		printk("## settimeofday not yet supported by so3\n");

		set_errno(-ENOSYS);
		result = -1;
		break;

	case SYSCALL_EXIT:
		do_exit(a->args[0]);
		break;

	case SYSCALL_EXECVE:
		result = do_execve((const char *)a->args[0],
				   (char **)a->args[1], (char **)a->args[2]);
		break;

	case SYSCALL_FORK:
		result = do_fork();
		break;

	case SYSCALL_WAITPID:
		result = do_waitpid(a->args[0], (uint32_t *)a->args[1],
				    a->args[2]);
		break;
#endif /* CONFIG_MMU */

	case SYSCALL_READ:
		result = do_read(a->args[0], (void *)a->args[1], a->args[2]);
		break;

	case SYSCALL_WRITE:
		result = do_write(a->args[0], (void *)a->args[1], a->args[2]);
		break;

	case SYSCALL_OPEN:
		result = do_open((const char *)a->args[0], a->args[1]);
		break;

	case SYSCALL_CLOSE:
		do_close((int)a->args[0]);
		result = 0;
		break;

	case SYSCALL_THREAD_CREATE:
		result = do_thread_create((uint32_t *)a->args[0], a->args[1],
					  a->args[2], a->args[3]);
		break;

	case SYSCALL_THREAD_JOIN:
		result = do_thread_join(a->args[0], (int **)a->args[1]);
		break;

	case SYSCALL_THREAD_EXIT:
		do_thread_exit((int *)a->args[0]);
		result = 0;
		break;

	case SYSCALL_THREAD_YIELD:
		do_thread_yield();
		result = 0;
		break;

	case SYSCALL_READDIR:
		result = do_readdir((int)a->args[0], (char *)a->args[1],
				    a->args[2]);
		break;

	case SYSCALL_IOCTL:
		result = do_ioctl((int)a->args[0], (unsigned long)a->args[1],
				  (unsigned long)a->args[2]);
		break;

	case SYSCALL_FCNTL:
		result = do_fcntl((int)a->args[0], (int)a->args[1],
				  (unsigned long)a->args[2]);
		break;

	case SYSCALL_LSEEK:
		result = do_lseek((int)a->args[0], (off_t)a->args[1],
				  (int)a->args[2]);
		break;

#ifdef CONFIG_IPC_PIPE
	case SYSCALL_PIPE:
		result = do_pipe((int *)a->args[0]);
		break;
#endif /* CONFIG_IPC_PIPE */

	case SYSCALL_DUP:
		result = do_dup((int)a->args[0]);
		break;

	case SYSCALL_DUP2:
		result = do_dup2((int)a->args[0], (int)a->args[1]);
		break;

	case SYSCALL_STAT:
		result = do_stat((char *)a->args[0], (struct stat *)a->args[1]);
		break;

	case SYSCALL_MMAP:
		result = (long)do_mmap((addr_t)a->args[0], (size_t)a->args[1],
				       (int)a->args[2], (int)a->args[3],
				       (off_t)a->args[4]);
		break;

	case SYSCALL_NANOSLEEP:
		result = do_nanosleep((const struct timespec *)a->args[0],
				      (struct timespec *)a->args[1]);
		break;

#ifdef CONFIG_PROC_ENV
	case SYSCALL_SBRK:
		result = do_sbrk((unsigned long)a->args[0]);
		break;

#endif /* CONFIG_PROC_ENV */

		/* This is a first attempt of mutex syscall implementation.
	 * Mainly used for debugging purposes (kernel mutex validation) at the moment ... */

	case SYSCALL_MUTEX_LOCK:
		result = do_mutex_lock(&current()->pcb->lock[a->args[0]]);
		break;

	case SYSCALL_MUTEX_UNLOCK:
		result = do_mutex_unlock(&current()->pcb->lock[a->args[0]]);
		break;

#ifdef CONFIG_MMU
	case SYSCALL_PTRACE:
		result = do_ptrace((enum __ptrace_request)a->args[0],
				   (uint32_t)a->args[1], (void *)a->args[2],
				   (void *)a->args[3]);
		break;
#endif

#ifdef CONFIG_IPC_SIGNAL
	case SYSCALL_SIGACTION:
		result = do_sigaction((int)a->args[0],
				      (sigaction_t *)a->args[1],
				      (sigaction_t *)a->args[2]);
		break;

	case SYSCALL_KILL:
		result = do_kill((int)a->args[0], (int)a->args[1]);
		break;

	case SYSCALL_SIGRETURN:
		do_sigreturn();
		break;

#endif /* CONFIG_IPC_SIGNAL */

#ifdef CONFIG_NET
	case SYSCALL_SOCKET:
		result = do_socket((int)a->args[0], (int)a->args[1],
				   (int)a->args[2]);
		break;

	case SYSCALL_BIND:
		result = do_bind((int)a->args[0],
				 (const struct sockaddr *)a->args[1],
				 (socklen_t)a->args[2]);
		break;

	case SYSCALL_LISTEN:
		result = do_listen((int)a->args[0], (int)a->args[1]);
		break;

	case SYSCALL_ACCEPT:
		result = do_accept((int)a->args[0],
				   (struct sockaddr *)a->args[1],
				   (socklen_t *)a->args[2]);
		break;

	case SYSCALL_CONNECT:
		result = do_connect((int)a->args[0],
				    (const struct sockaddr *)a->args[1],
				    (socklen_t)a->args[2]);
		break;

	case SYSCALL_RECV:
		result = do_recv((int)a->args[0], (void *)a->args[1],
				 (size_t)a->args[2], (int)a->args[3]);
		break;

	case SYSCALL_SEND:
		result = do_send((int)a->args[0], (const void *)a->args[1],
				 (size_t)a->args[2], (int)a->args[3]);
		break;

	case SYSCALL_SENDTO:
		result = do_sendto((int)a->args[0], (const void *)a->args[1],
				   (size_t)a->args[2], (int)a->args[3],
				   (const struct sockaddr *)a->args[4],
				   (socklen_t)a->args[5]);
		break;

	case SYSCALL_SETSOCKOPT:
		result = do_setsockopt((int)a->args[0], (int)a->args[1],
				       (int)a->args[2],
				       (const void *)a->args[3],
				       (socklen_t)a->args[4]);
		break;

	case SYSCALL_RECVFROM:
		result = do_recvfrom((int)a->args[0], (void *)a->args[1],
				     (size_t)a->args[2], (int)a->args[3],
				     (struct sockaddr *)a->args[4],
				     (socklen_t *)a->args[5]);
		break;

#endif /* CONFIG_NET */

	/* Sysinfo syscalls */
	case SYSCALL_SYSINFO:
		switch (a->args[0]) {
		case SYSINFO_DUMP_HEAP:
			dump_heap("Heap info asked from user.\n");
			break;

		case SYSINFO_DUMP_SCHED:
			dump_sched();
			break;

#ifdef CONFIG_MMU
		case SYSINFO_DUMP_PROC:
			dump_proc();
			break;
#endif

#ifdef CONFIG_APP_TEST_MALLOC
		case SYSINFO_TEST_MALLOC:
			test_malloc(a->args[1]);
			break;
#endif
		case SYSINFO_PRINTK:
			printk("%s", (char *)a->args[1]);
			break;
		}
		result = 0;
		break;

	default:
		printk("%s: unhandled syscall: %d\n", __func__, syscall_no);
		break;
	}

#warning do_softirq?

	return result;
}
