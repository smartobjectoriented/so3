/*
 * Copyright (C) 2025 Clement Dieperink <clement.dieperink@heig-vd.ch>
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

#ifndef ARCH_ARM32_SYSCALL_NUMBER_H
#define ARCH_ARM32_SYSCALL_NUMBER_H

/*
 * Syscall number definition
 */

#define SYSCALL_EXIT 1
#define SYSCALL_FORK 2
#define SYSCALL_READ 3
#define SYSCALL_WRITE 4
#define SYSCALL_OPEN 5 // Add mode
#define SYSCALL_CLOSE 6

#define SYSCALL_EXECVE 11

#define SYSCALL_LSEEK 19
#define SYSCALL_GETPID 20

#define SYSCALL_PTRACE 26

#define SYSCALL_KILL 37

#define SYSCALL_DUP 41

#define SYSCALL_BRK 45
#define SYSCALL_PIPE 46

#define SYSCALL_IOCTL 54
#define SYSCALL_FCNTL 55 // fcntl64?

#define SYSCALL_DUP2 63
#define SYSCALL_SIGACTION 67

#define SYSCALL_GETTIMEOFDAY_TIME32 78
// #define SYSCALL_SETTIMEOFDAY 79 // Implement?

#define SYSCALL_STAT 106

// #define SYSCALL_SYSINFO 116 // => struct sysinfo

#define SYSCALL_SIGRETURN 119

#define SYSCALL_READV 145
#define SYSCALL_WRITEV 146

#define SYSCALL_NANOSLEEP 162

#define SYSCALL_GETDENTS64 217

#define SYSCALL_WAIT4 260

#define SYSCALL_CLOCK_GETTIME32 263

#define SYSCALL_SOCKET 281
#define SYSCALL_BIND 282
#define SYSCALL_CONNECT 283
#define SYSCALL_LISTEN 284
#define SYSCALL_ACCEPT 285

#define SYSCALL_SEND 289
#define SYSCALL_SENDTO 290

#define SYSCALL_RECV 291
#define SYSCALL_RECVFROM 292

#define SYSCALL_SETSOCKOPT 294

#define SYSCALL_DUP3 358

#define SYSCALL_CLOCK_GETTIME64 403

// Does not exist
#define SYSCALL_MMAP 222 // => mmap2

// => pthread
#define SYSCALL_THREAD_CREATE 16
#define SYSCALL_THREAD_JOIN 17
#define SYSCALL_THREAD_EXIT 18
#define SYSCALL_THREAD_YIELD 43

// => ???
#define SYSCALL_MUTEX_LOCK 60
#define SYSCALL_MUTEX_UNLOCK 61

#endif /* ARCH_ARM32_SYSCALL_NUMBER_H */
