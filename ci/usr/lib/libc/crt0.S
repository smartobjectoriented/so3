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

#include <syscall.h>

.extern __entryC

.data
.globl	errno

.align 8
errno:
	.word	0x0

.text


/* -------------------------------------------------------------
 * __start
 *	Initialize running a C program, by calling "main".
 * -------------------------------------------------------------
 */

	.globl	__start
	.section ".head", "ax"
__start:

	/* r0 points to the argument page */

	bl		__entryC

	b		exit	 /* if we return from main, exit(return value) */

	nop

/* -------------------------------------------------------------
 * System call stubs:
 * - r0-r3 are used to store arguments
 * - r7 is used to store the syscall number
 * -------------------------------------------------------------
 */

#ifdef __ARM__

.macro SYSCALLSTUB name, number
	.globl	\name
\name:

  stmfd sp!, {r7, r10, ip}

  mov	r7, #\number

  ldr	r10, =errno
  mov	ip, #0
  str	ip, [r10]

  swi	0

  ldmfd sp!, {r7, r10, ip}

  mov pc, lr

.endm

#else /* __ARM64__ */

.macro SYSCALLSTUB name, number
	.globl	\name
\name:

  // 16-byte stack alignment
  sub	sp, sp, #32

  str	x8, [sp]
  str	x9, [sp, #8]
  str	lr,  [sp, #16]

  mov	x8, #\number

  ldr	x9, =errno
  str	wzr, [x9]

  svc	0

  ldr 	x8, [sp]
  ldr	x9, [sp, #8]
  ldr	lr, [sp, #16]

  add	sp, sp, #32

  ret

.endm


#endif /* __ARM64__ */


/* Syscalls stubs */
SYSCALLSTUB sys_halt, 			syscallHalt
SYSCALLSTUB sys_write,			syscallWrite
SYSCALLSTUB sys_read, 			syscallRead
SYSCALLSTUB sys_exit, 			syscallExit
SYSCALLSTUB sys_execve,			syscallExecve
SYSCALLSTUB sys_waitpid,		syscallWaitpid
SYSCALLSTUB sys_pause, 			syscallPause
SYSCALLSTUB sys_fork, 			syscallFork
SYSCALLSTUB sys_readdir, 		syscallReaddir
SYSCALLSTUB sys_chdir, 			syscallChdir
SYSCALLSTUB sys_getcwd, 		syscallGetcwd
SYSCALLSTUB sys_creat, 			syscallCreate
SYSCALLSTUB sys_unlink, 		syscallUnlink
SYSCALLSTUB sys_open, 			syscallOpen
SYSCALLSTUB sys_close, 			syscallClose
SYSCALLSTUB sys_thread_create, 	syscallThreadCreate
SYSCALLSTUB sys_thread_join, 	syscallThreadJoin
SYSCALLSTUB sys_thread_exit, 	syscallThreadExit
SYSCALLSTUB sys_thread_yield,   syscallThreadYield
SYSCALLSTUB sys_pipe, 			syscallPipe
SYSCALLSTUB sys_ioctl,			syscallIoctl
SYSCALLSTUB sys_fcntl,	 	    syscallFcntl
SYSCALLSTUB sys_stat,	 		syscallStat
SYSCALLSTUB sys_dup,	 		syscallDup
SYSCALLSTUB sys_dup2,	 		syscallDup2
SYSCALLSTUB sys_sched_setparam, syscallSchedSetParam
SYSCALLSTUB sys_socket, 		syscallSocket
SYSCALLSTUB sys_bind, 			syscallBind
SYSCALLSTUB sys_listen, 		syscallListen
SYSCALLSTUB sys_accept, 		syscallAccept
SYSCALLSTUB sys_connect, 		syscallConnect
SYSCALLSTUB sys_mmap, 			syscallMmap
SYSCALLSTUB sys_ptrace,  		syscallPtrace
SYSCALLSTUB sys_send,	 		syscallSend
SYSCALLSTUB sys_recv, 			syscallRecv
SYSCALLSTUB sys_recvfrom, 		syscallRecvfrom
SYSCALLSTUB sys_setsockopt, 	syscallSetsockopt
SYSCALLSTUB sys_sendto, 		syscallSendTo
SYSCALLSTUB sys_getpid,			syscallGetpid

SYSCALLSTUB sys_gettimeofday,	syscallGetTimeOfDay
SYSCALLSTUB sys_settimeofday,	syscallSetTimeOfDay
SYSCALLSTUB sys_clock_gettime,  syscallClockGetTime

SYSCALLSTUB sys_sbrk,			syscallSbrk
SYSCALLSTUB sys_info,	    	syscallSysinfo

SYSCALLSTUB sys_lseek,			syscallLseek

SYSCALLSTUB sys_mutex_lock,	    syscallMutexLock
SYSCALLSTUB sys_mutex_unlock,	syscallMutexUnlock

SYSCALLSTUB sys_sigaction,		syscallSigaction
SYSCALLSTUB sys_kill,			syscallKill
SYSCALLSTUB sys_sigreturn,		syscallSigreturn

SYSCALLSTUB sys_nanosleep,		syscallNanosleep


