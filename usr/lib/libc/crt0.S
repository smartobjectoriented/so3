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

#ifdef __ARM__

/* -------------------------------------------------------------
 * System call stubs:
 * - r0-r3 are used to store arguments
 * - Further argument are on stack and will be put to r4-r5 if needed.
 * - r7 is used to store the syscall number
 * -------------------------------------------------------------
 */

.macro SYSCALLSTUB name, number, nargs
	.globl	\name
\name:

  stmfd sp!, {r4, r5, r7, r10, ip}

/* Retrieve arguments 5 and 6 from the stack if needed */
.ifge	\nargs - 5
  ldr	r4, [sp, #20]
.endif
.ifge	\nargs - 6
  ldr	r5, [sp, #24]
.endif

  mov	r7, #\number

  ldr	r10, =errno
  mov	ip, #0
  str	ip, [r10]

  swi	0

  ldmfd sp!, {r4, r5, r7, r10, ip}

  mov pc, lr

.endm

#else /* __ARM64__ */

/* -------------------------------------------------------------
 * System call stubs:
 * - x0-x5 are used to store arguments
 * - x8 is used to store the syscall number
 * -------------------------------------------------------------
 */

.macro SYSCALLSTUB name, number, nargs
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

  ldr	x8, [sp]
  ldr	x9, [sp, #8]
  ldr	lr, [sp, #16]

  add	sp, sp, #32

  ret

.endm


#endif /* __ARM64__ */


/* Syscalls stubs */
SYSCALLSTUB sys_halt,			syscallHalt		0
SYSCALLSTUB sys_write,			syscallWrite		3
SYSCALLSTUB sys_read,			syscallRead		3
SYSCALLSTUB sys_exit,			syscallExit		1
SYSCALLSTUB sys_execve,			syscallExecve		3
SYSCALLSTUB sys_waitpid,		syscallWaitpid		3
SYSCALLSTUB sys_pause,			syscallPause		1
SYSCALLSTUB sys_fork,			syscallFork		0
SYSCALLSTUB sys_readdir,		syscallReaddir		3
/* SYSCALLSTUB sys_chdir,			syscallChdir  */
/* SYSCALLSTUB sys_getcwd,			syscallGetcwd */
SYSCALLSTUB sys_creat,			syscallCreate		2
SYSCALLSTUB sys_unlink,			syscallUnlink		1
SYSCALLSTUB sys_open,			syscallOpen		3
SYSCALLSTUB sys_close,			syscallClose		1
SYSCALLSTUB sys_thread_create,		syscallThreadCreate	4
SYSCALLSTUB sys_thread_join,		syscallThreadJoin	2
SYSCALLSTUB sys_thread_exit,		syscallThreadExit	1
SYSCALLSTUB sys_thread_yield,		syscallThreadYield	0
SYSCALLSTUB sys_pipe,			syscallPipe		1
SYSCALLSTUB sys_ioctl,			syscallIoctl		3
SYSCALLSTUB sys_fcntl,			syscallFcntl		3
SYSCALLSTUB sys_stat,			syscallStat		2
SYSCALLSTUB sys_dup,			syscallDup		1
SYSCALLSTUB sys_dup2,			syscallDup2		2
SYSCALLSTUB sys_sched_setparam,		syscallSchedSetParam	2
SYSCALLSTUB sys_socket,			syscallSocket		3
SYSCALLSTUB sys_bind,			syscallBind		3
SYSCALLSTUB sys_listen,			syscallListen		2
SYSCALLSTUB sys_accept,			syscallAccept		3
SYSCALLSTUB sys_connect,		syscallConnect		3
SYSCALLSTUB sys_mmap,			syscallMmap		5
SYSCALLSTUB sys_ptrace,			syscallPtrace		4
SYSCALLSTUB sys_send,			syscallSend		4
SYSCALLSTUB sys_recv,			syscallRecv		4
SYSCALLSTUB sys_recvfrom,		syscallRecvfrom		6
SYSCALLSTUB sys_setsockopt,		syscallSetsockopt	5
SYSCALLSTUB sys_sendto,			syscallSendTo		6
SYSCALLSTUB sys_getpid,			syscallGetpid		0

SYSCALLSTUB sys_gettimeofday,		syscallGetTimeOfDay	2
SYSCALLSTUB sys_settimeofday,		syscallSetTimeOfDay	2
SYSCALLSTUB sys_clock_gettime,		syscallClockGetTime	2

SYSCALLSTUB sys_sbrk,			syscallSbrk		1
SYSCALLSTUB sys_info,			syscallSysinfo		2

SYSCALLSTUB sys_lseek,			syscallLseek		3

SYSCALLSTUB sys_mutex_lock,		syscallMutexLock	1
SYSCALLSTUB sys_mutex_unlock,		syscallMutexUnlock	1

SYSCALLSTUB sys_sigaction,		syscallSigaction	3
SYSCALLSTUB sys_kill,			syscallKill		2
SYSCALLSTUB sys_sigreturn,		syscallSigreturn	0

SYSCALLSTUB sys_nanosleep,		syscallNanosleep	2


