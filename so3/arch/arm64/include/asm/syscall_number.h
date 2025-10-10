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

#ifndef ARCH_ARM64_SYSCALL_NUMBER_H
#define ARCH_ARM64_SYSCALL_NUMBER_H

/*
 * Syscall number definition
 */
#define SYSCALL_DUP 23
#define SYSCALL_DUP3 24

#define SYSCALL_IOCTL 29

#define SYSCALL_OPENAT 56
#define SYSCALL_CLOSE 57

#define SYSCALL_PIPE2 59

#define SYSCALL_GETDENTS64 61
#define SYSCALL_LSEEK 62
#define SYSCALL_READ 63
#define SYSCALL_WRITE 64
#define SYSCALL_READV 65
#define SYSCALL_WRITEV 66

#define SYSCALL_FSTATAT 79

#define SYSCALL_EXIT 93

#define SYSCALL_NANOSLEEP 101

#define SYSCALL_CLOCK_GETTIME 113
#define SYSCALL_WAIT4 114

#define SYSCALL_PTRACE 117

#define SYSCALL_KILL 129

#define SYSCALL_RT_SIGACTION 134

#define SYSCALL_RT_SIGRETURN 139

#define SYSCALL_GETTIMEOFDAY 169

#define SYSCALL_GETPID 172

#define SYSCALL_SOCKET 198

#define SYSCALL_BIND 200
#define SYSCALL_LISTEN 201
#define SYSCALL_ACCEPT 202
#define SYSCALL_CONNECT 203

#define SYSCALL_SENDTO 206
#define SYSCALL_RECVFROM 207
#define SYSCALL_SETSOCKOPT 208

#define SYSCALL_BRK 214

#define SYSCALL_EXECVE 221
#define SYSCALL_MMAP 222

/* Following syscalls stills need to be align */
#define SYSCALL_FORK 7 // => clone

#define SYSCALL_THREAD_CREATE 16
#define SYSCALL_THREAD_JOIN 17
#define SYSCALL_THREAD_EXIT 18
#define SYSCALL_THREAD_YIELD 43

#define SYSCALL_MUTEX_LOCK 60
#define SYSCALL_MUTEX_UNLOCK 61

#endif /* ARCH_ARM64_SYSCALL_NUMBER_H */
