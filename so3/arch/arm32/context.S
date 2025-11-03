/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

@ Manage various context-related code (context switch)

#include <generated/autoconf.h>

#include <asm/assembler.h>
#include <asm-offsets.h>
#include <asm/processor.h>
#include <asm/mmu.h>

#include <syscall.h>
#include <vfs.h>
#include <linkage.h>

.global __switch_context
.global __thread_prologue_kernel
.global __exec_prologue_user

.globl __get_syscall_args_ext
.globl __get_syscall_arg

.global __mmu_switch_ttbr0
.global __exec
.global __write

.global __enable_vfp

.extern thread_prologue
.extern current_thread

#ifdef CONFIG_MMU

.extern __check_ptrace_traceme
.extern pre_launch_proc

#endif

@ Enable VFP
__enable_vfp:

    mov r1, #0x40000000
    vmsr fpexc, r1 ; fpexc = r1
    bx lr

@ Get the additional arguments linked to the syscall.
@ The ABI convention is described in crt0.S of the libc.
@ r7 contains the syscall number
__get_syscall_args_ext:

	str		r7, [r0]

	mov 	pc, lr

@ Kernel thread initial entry point
@ Called once per thread
__thread_prologue_kernel:

	@ Prepare to jump into C code
	mov r0, r4 		@ tcb->th_fn
	mov r1, r5 		@ tcb->th_arg

	cpsie   i

	bl	thread_prologue

#ifdef CONFIG_AVZ

ENTRY(cpu_do_idle)

	dsb	  sy			// WFI may enter a low-power mode
	wfi

	mov pc,	lr

/*
 * r0 = previous domain, r1 = next domain
 * previous and next are guaranteed not to be the same.
 *
 */
ENTRY(__switch_domain_to)
	disable_irq 				@ ensure IRQs are disabled

	add     ip, r0, #(OFFSET_CPU_REGS + OFFSET_R4)
@ save_ctx
    stmia   ip, {r4 - sl, fp, ip, sp, lr}      @ Store most regs on stack

	current_cpu r6

@ load_ctx

	add		ip, r1, #(OFFSET_CPU_REGS + OFFSET_R4)

    ldmia   ip,  {r4 - sl, fp, ip, sp, pc}       @ Load all regs saved previously

	nop
	nop
	nop

#endif /* CONFIG_AVZ */

@ r0: address of tcb prev
@ r1: address of tcb next
@ IRQs are disabled and SVC mode
ENTRY(__switch_to)

	cmp	r0, #0
	beq	load_ctx

	add     ip, r0, #(OFFSET_TCB_CPU_REGS + OFFSET_R4)
save_ctx:

	stmia   ip, {r4 - r10, fp, ip, sp, lr}      @ Store most regs on stack

load_ctx:

	add		ip, r1, #(OFFSET_TCB_CPU_REGS + OFFSET_R4)

	ldmia   ip,  {r4 - r10, fp, ip, sp, pc}       @ Load all regs saved previously

	nop
	nop
	nop

@ Switch the MMU to a L1 page table
@ r0 contains the physical address of the L1 page table
@ r1 and r2 are used as scratch registers.

__mmu_switch_ttbr0:

	dsb
	isb

	@ So far, we do not use ASID/Context ID

	mrc p15, 0, r1, c2, c0, 0
	isb

	ldr	r2, =TTBR0_BASE_ADDR_MASK
	bic r1, r1, r2

	orr r1, r1, r0

	mcr	p15, 0, r1, c2, c0, 0		@ set TTBR0

	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0		@ I+BTB cache invalidate

	dsb
	isb

	mov		pc, lr

	nop
	nop
	nop

.LCcurrent:
	.word current_thread

@ This section contains all data and code which will be
@ mapped in the user space as initial process to be executed.

.section ".root_proc.text","ax"

	b	__root_proc

.LC_welcome:
	.string "[SO3 USR] Starting the initial process\n"
	.equ welcome_len, . - .LC_welcome

.LC_exec:
	.string CONFIG_USER_INIT_PROGRAM

.LCnoshell:
	.string "[SO3 USR] No init process found\n"
	.equ noshell_len, . - .LCnoshell


.align 2

@ This is the initial code which runs in user mode
@ to start the first process image

ENTRY(__root_proc)

	@ write(stdout, buf, len)
	mov r0, #STDOUT
	adr r1, .LC_welcome
	mov r2, #welcome_len
	mov r7, #SYSCALL_write
	@ Invoke the syscall - kernel side
	svc	0

	@ exec("sh.elf")
	adr r0, .LC_exec
	@ No args
	mov r1, #0
	mov r2, #0

	mov	r7, #SYSCALL_execve

	svc 0
