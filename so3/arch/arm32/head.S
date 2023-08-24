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
#include <sizes.h>

#include <generated/autoconf.h>

#include <asm/processor.h>
#include <asm/mmu.h>

#include <thread.h>

.global __start
.global __fdt_addr

.extern clear_bss
.extern mmu_configure
.extern __stack_top
.extern __vectors
.extern __bss_start, __bss_end

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

__start:

  @ r1 = machine id
  @ r2 = dtb address

  @ Make sure we start in SVC mode
#ifdef CONFIG_SO3VIRT

  ldr	r10, =avz_shared
  str 	r12, [r10]

  @ For future conversion to physical which is not required in this case.
  mov   r8, #0

#else /* !CONFIG_SO3VIRT */
  safe_svcmode_maskall r9

  msr  	cpsr_c, #PSR_F_BIT | PSR_I_BIT | PSR_SVC_MODE @ ensure svc mode and irqs disabled

  @ Determine the offset to use for converting virtual to physical (virtual address compiled)
  ldr  r8, =__virtaddr
  sub  r8, pc, r8

  b cont

__virtaddr:
	.word

cont:

  @ Configure address of the vector table
  @ The pseudo is using the current PC so we are with the physical address here.

  ldr 	r1, =__vectors
  add	r1, r1, r8
  mcr 	p15, 0, r1, c12, c0, 0		@ vector table

#endif

  @ Prepare to clear bss and zero fp
  ldr	r6, =__bss_start
  ldr	r7, =__bss_end

  # Convert to physical address
  add	r6, r6, r8
  add	r7, r7, r8

  mov	fp, #0			@ Clear BSS (and zero fp)
1:
  cmp	r6, r7
  strcc	fp, [r6], #4
  bcc	1b

  @ Preserve the (physical address of) device tree base in r9
  mov 	r9, r2

  @ Take a initial stack not too far away from this code (see comment below)
  ldr  	sp, =temp_stack

  @ And convert to physical address
  add	sp, sp, r8

  @ Up to here, a stack should be initialized

#ifdef CONFIG_MMU

  @ Set up the MMU and other flags of the system control register
  b	mmu_setup

#endif /* CONFIG_MMU */

__kernel_main:

  @ Basic low-level initialization
  mov 	r0, r9
  bl 	early_memory_init

#if defined(CONFIG_SOO) && !defined(CONFIG_AVZ)
  bl 	avz_setup
#else
  bl 	setup_arch
#endif

  @ C main entry point
#ifdef CONFIG_AVZ
	b 	avz_start
#else
	b 	kernel_start
#endif

  @ never returns...


#ifdef CONFIG_MMU

  .align 2

mmu_setup:

  add	r0, r8, #CONFIG_KERNEL_VADDR
  mov	r1, r9 @ fdt addr

  bl 	mmu_configure

  @ Readjust the stack
  ldr	sp, =__stack_top

  @ Keep executing in the kernel space

  @ Store the virtual address which will be used to continue
  @ the execution after the MMU enabled.
  ldr	r0, =.LCvirt_entry
  ldr	pc, [r0]

#endif /* CONFIG_MMU */

#ifdef CONFIG_SMP

ENTRY(secondary_startup)
	/*
     * Common entry point for secondary CPUs.
     *
     * Ensure that we're in SVC mode, and IRQs are disabled.  Lookup
     * the processor type - there is no need to check the machine type
     * as it has already been validated by the primary processor.
     */

    safe_svcmode_maskall r9

  	/*
     * Use the page tables supplied from  __cpu_up.
     */
    adr 	r4, __secondary_data
    ldmia   r4, {r5, r7}       @ retrieve secondary_data field values (pgdir, stack) / r5 & r7 are virtual addresses.
    sub 	lr, r4, r5         @ r4 is a physical address since the adr pseudo is based on relative pc (mmu is off at this point).

    ldr 	r0, [lr, r7]       @ get secondary_data.pgdir (r7 is virtual, r0 is phys).
    add		r7, r7, #4
    ldr		sp, [lr, r7]	   @ get the stack pointer (virt. address)
	add		sp, sp, lr		   @ convert to phys address.

  	@ Up to here, a stack should be initialized

  	@ Set up the MMU - The second argument (fdt addr) is not used in this context.
  	bl 	mmu_configure

 	 @ Readjust the stack (r7 has still the virt. address of the stack)
  	ldr	sp, [r7]

  	@ Keep executing in the kernel space

  	@ Store the virtual address which will be used to continue
 	@ the execution after the MMU enabled.
 	ldr	r0, =.LC_virt_secondary_entry
  	ldr	pc, [r0]

    .align

    .type   __secondary_data, %object
__secondary_data:
    .long   .
    .long   secondary_data

#endif /* CONFIG_SMP */

.ltorg

__fdt_addr:
  .word 	0

.align 13

@ Before MMU is enabled, we cannot refer to the normal stack as declared 
@ in the linker script because the stack is too far away from this code
@ and will not be within the 1-MB direct mapping after the MMU is enabled.

temp_stack_bottom:
	.space 4096
temp_stack:

.LCvirt_entry:
  .word __kernel_main

#ifdef CONFIG_SMP

.LC_virt_secondary_entry:
  .word secondary_start_kernel

#endif /* CONFIG_SMP */

.LCsys_root_pgtable:
  .word __sys_root_pgtable
