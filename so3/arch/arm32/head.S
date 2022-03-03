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

@ SO3 Kernel entry point

#include <linkage.h>

#include <generated/autoconf.h>

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/memory.h>
#include <thread.h>

.global _start
.global __fdt_addr

.extern clear_bss
.extern mmu_configure
.extern __stack_top
.extern __vectors_start

.section ".head.text","ax"


/*
 * Helper macro to enter SVC mode cleanly and mask interrupts. reg is
 * a scratch register for the macro to overwrite.
 *
 * This macro is intended for forcing the CPU into SVC mode at boot time.
 * you cannot return to the original mode.
 */
.macro safe_svcmode_maskall reg:req

	mrs	\reg , cpsr
	eor	\reg, \reg, #PSR_HYP_MODE
	tst	\reg, #PSR_MODE_MASK
	bic	\reg , \reg , #PSR_MODE_MASK
	orr	\reg , \reg , #PSR_I_BIT | PSR_F_BIT | PSR_SVC_MODE

	bne	1f
	orr	\reg, \reg, #PSR_A_BIT
	adr	lr, 2f
	msr	spsr_cxsf, \reg

	__MSR_ELR_HYP(14)
	__ERET

1:	msr	cpsr_c, \reg
2:

.endm

_start:

  @ r1 = machine id
  @ r2 = dtb address

  @ Make sure we start in SVC mode

  safe_svcmode_maskall r9

  msr  	cpsr_c, #PSR_F_BIT | PSR_I_BIT | PSR_SVC_MODE @ ensure svc mode and irqs disabled

  @ Configure address of the vector table
  ldr 	r1, =.LCvectors
  ldr	r1, [r1]

  mcr 	p15, 0, r1, c12, c0, 0		@ vector table

  @ prepare to clear bss and zero fp
  ldr	r6, =__bss_start
  ldr	r7, =__bss_end

  mov	fp, #0			@ Clear BSS (and zero fp)
1:
  cmp	r6, r7
  strcc	fp, [r6], #4
  bcc	1b

  @ Preserve the (physical address of) device tree base in r9
  mov 	r9, r2

  @ Initialize stack pointers for current mode (normal case if no MMU is used)
  ldr  	sp, =.LCstack_top
  ldr 	sp, [sp]

  @ Up to here, a stack should be initialized

#ifdef CONFIG_MMU

  @ Set up the MMU and other flags of the system control register
  b		mmu_setup

#endif /* CONFIG_MMU */

__kernel_main:

  ldr	r0, =__fdt_addr
  str	r9, [r0]

  @ C main entry point
  b 	kernel_start

  @ never returns...


#ifdef CONFIG_MMU

  .align 2

mmu_setup:

  @ Use a temporary stack
  adrl	sp, temp_stack

  @ Refer to the 1st-level page table
  ldr	r4, =CONFIG_RAM_BASE + TTB_L1_SYS_OFFSET

  mov	r0, r4
  mov	r1, r9 @ fdt addr

  bl mmu_configure

  @ Readjust the stack
  ldr	sp, =.LCstack_top
  ldr	sp, [sp]

  @ Keep executing in the kernel space

  @ Store the virtual address which will be used to continue
  @ the execution after the MMU enabled.
  ldr	r0, =.LCvirt_entry
  ldr	pc, [r0]

 #endif /* CONFIG_MMU */


.ltorg

__fdt_addr:
  .word 	0

@ Keep the stack aligned on 8 KB according to EABI
.align 13

@ Before MMU is enabled, we cannot refer to the normal stack as declared in the linker script
temp_stack_bottom:
	.space 4096
temp_stack:

.LCvirt_entry:
  .word __kernel_main

.LCstack_top:
  .word __stack_top

.LCvectors:
  .word __vectors_start
