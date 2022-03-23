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

#define SYSINFO_DUMP_HEAP	0
#define SYSINFO_DUMP_SCHED	1
#define SYSINFO_TEST_MALLOC	2
#define SYSINFO_PRINTK		3
#define SYSINFO_DUMP_PROC	4

/*
 * Syscall number definition
 */

#define SYSCALL_EXIT		1
#define SYSCALL_EXECVE 		2
#define SYSCALL_WAITPID		3
#define SYSCALL_READ 		4
#define SYSCALL_WRITE 		5
#define SYSCALL_FORK 		7
#define SYSCALL_PTRACE		8
#define SYSCALL_READDIR		9
#define SYSCALL_OPEN 		14
#define SYSCALL_CLOSE		15
#define SYSCALL_THREAD_CREATE	16
#define SYSCALL_THREAD_JOIN	17
#define SYSCALL_THREAD_EXIT	18
#define SYSCALL_PIPE 		19
#define SYSCALL_IOCTL		20
#define SYSCALL_FCNTL		21
#define SYSCALL_DUP		22
#define SYSCALL_DUP2		23

#define SYSCALL_SOCKET 		26
#define SYSCALL_BIND		27
#define SYSCALL_LISTEN		28
#define SYSCALL_ACCEPT 		29
#define SYSCALL_CONNECT		30
#define SYSCALL_RECV		31
#define SYSCALL_SEND		32
#define SYSCALL_SENDTO		33

#define SYSCALL_STAT		34
#define SYSCALL_MMAP		35
#define SYSCALL_GETPID		37

#define SYSCALL_GETTIMEOFDAY	38
#define SYSCALL_SETTIMEOFDAY	39
#define SYSCALL_CLOCK_GETTIME	40

#define SYSCALL_THREAD_YIELD	43

#define SYSCALL_SBRK		45
#define SYSCALL_SIGACTION	46
#define SYSCALL_KILL		47
#define SYSCALL_SIGRETURN	48

#define SYSCALL_LSEEK		50

#define SYSCALL_MUTEX_LOCK	60
#define SYSCALL_MUTEX_UNLOCK	61

#define SYSCALL_NANOSLEEP	70

#define SYSCALL_SYSINFO		99

#define SYSCALL_SETSOCKOPT	110
#define SYSCALL_RECVFROM	111

#ifndef __ASSEMBLY__

#include <errno.h>
#include <types.h>

long syscall_handle(unsigned long, unsigned long, unsigned long, unsigned long);

void set_errno(uint32_t val);
#endif /* __ASSEMBLY__ */

#endif /* ASM_ARM_SYSCALL_H */
