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
#include <generated/autoconf.h>

#include <common.h>

#include <linkage.h>

#include <device/arch/gic.h>

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/image.h>
#include <asm/virt.h>

.global __start
.global __fdt_addr
.global __mmu_setup

.extern clear_bss
.extern mmu_configure
.extern __stack_top
.extern __vectors

.section ".head.text","ax"

__pre_head:
	b	__start					// branch to the main entry point
	.long	0					// reserved

	.quad	0					// Image load offset from start of RAM, little-endian
	.quad	__end - __start		// Effective size of kernel image, little-endian
	.quad	__HEAD_FLAGS		// Informative flags, little-endian
	.quad	0					// reserved
	.quad	0					// reserved
	.quad	__end				// reserved
	.ascii	__HEAD_MAGIC		// Magic number
	.long	0					// reserved

__start:

#ifndef CONFIG_SO3VIRT
	// Preserve the fdt addr (device tree) which is stored in x0 by U-boot
	mov		x9, x0
#else
	mov		x9, x21
#endif

	//Store the DT addr in a safe place which can be accessed on a relocatable address
	adr     x8, .LCfdt_addr_temp
	str 	x9, [x8]

#ifndef CONFIG_SO3VIRT
	// Mostly for future usage...
	bl		el2_setup			// Drop to EL1, w0=cpu_boot_mode

#endif /* !CONFIG_SO3VIRT */

	/* Clear the BSS */

	adrp	x0, __bss_start
	adrp	x1, __bss_end
1:
	strb	wzr, [x0], #1

	cmp		x0, x1
	b.cc	1b

#ifndef CONFIG_SO3VIRT

	// Initialize the vector table
	ldr	x0, =__vectors

#ifdef CONFIG_ARM64VT
	msr vbar_el2, x0
#else
	msr vbar_el1, x0
#endif /* !CONFIG_ARM64VT */

	// Enable FP/ASIMD
	mov	x0, #3 << 20
	msr	cpacr_el1, x0

#endif /* !CONFIG_SO3VIRT */

    // Up to here, a stack should be initialized

#ifdef CONFIG_SO3VIRT

  	ldr	x10, =avz_shared
  	str x22, [x10]

	bl	avz_setup

#endif /* CONFIG_SO3VIRT */

	
    // Set up the MMU
    b setup_mmu

__kernel_main:

	// Store the device tree paddr
	adr x8, .LCfdt_addr_temp
	ldr x9, [x8]

	ldr	x0, =__fdt_addr
	str	x9, [x0]

	// C main entry point
#ifdef CONFIG_AVZ
	b 	avz_start
#else
	b 	kernel_start
#endif

	nop
	nop
	nop

	// never returns...

.LCfdt_addr_temp:
	.quad 0

.align 2

#ifdef CONFIG_ARM64VT
__mmu_setup:
		// The code to configure tcr_el2
		// is borrowed from jailhouse hypervisor
	
		mrs		x1, tcr_el2
	
		/*
		 * set TCR.(I)PS to the highest supported ID_AA64MMFR0_EL1.PARange value
		 */

		mrs     x2, id_aa64mmfr0_el1

		/* Narrow PARange to fit the PS field in TCR_ELx */
		ubfx    x2, x2, #ID_AA64MMFR0_PARANGE_SHIFT, #3
		bfi     x1, x2, #TCR_PS_SHIFT, #3

		msr		tcr_el2, x1

		/*
		 * The state of the TLBs is unknown before turning on the MMU.
		 * Flush them to avoid stale one.
		 */
		tlbi  alle2                  /* Flush hypervisor TLBs */
		dsb   nsh

		msr   ttbr0_el2, x0
		isb

		ldr	x0, =(SCTLR_I_BIT | SCTLR_C_BIT | SCTLR_M_BIT | SCTLR_EL2_RES1)

		dsb   sy                     /* Flush PTE writes and finish reads */
		msr   sctlr_el2, x0          /* now paging is enabled */
		isb                          /* Now, flush the icache */

		ret

setup_mmu:
		// Use a temporary stack
		adrp	x0, temp_stack
		mov		sp, x0

		mov		x0, x9 // fdt addr

		bl 		mmu_configure

		// Readjust the stack
		ldr		x0, =__stack_top
		mov		sp, x0

		// Keep executing in the kernel space

		// Store the virtual address which will be used to continue
		// the execution after the MMU enabled.
		ldr		x0, .LCvirt_entry

		blr		x0

el2_setup:

		/* Populate ID registers. */
		mrs	x0, midr_el1
		mrs	x1, mpidr_el1
		msr	vpidr_el2, x0
		msr	vmpidr_el2, x1

		msr	sctlr_el1, xzr

		/* Hyp configuration. */
	    and x1, x1, #3

#ifdef CONFIG_SOO
		cmp x1, #AGENCY_RT_CPU
		bgt ME
#endif /* CONFIG_SOO */

agency:
	ldr x0, =HCR_AGENCY_FLAGS
	b	99f
ME:
	ldr x0, =HCR_ME_FLAGS
99:

	msr	hcr_el2, x0
	isb

	msr	cntvoff_el2, xzr		// Clear virtual offset

    /*
     * Ensure that any exceptions encountered at EL2
     * are handled using the EL2 stack pointer, rather
     * than SP_EL0.
    */
    msr spsel, #1

	ret

#else /* CONFIG_ARM64VT */

setup_mmu:

	// Use a temporary stack
  	adrp x0, temp_stack
  	mov	sp, x0

#ifndef CONFIG_SO3VIRT
  	bl 	mmu_configure
#endif

  	// Readjust the stack
  	ldr	x0, =__stack_top

  	mov	sp, x0

  	// Keep executing in the kernel space

  	// Store the virtual address which will be used to continue
  	// the execution after the MMU enabled.
  	ldr	x0, .LCvirt_entry

  	blr	x0

/*
 * If we're fortunate enough to boot at EL2, ensure that the world is
 * sane before dropping to EL1.
 *
 * Returns either BOOT_CPU_MODE_EL1 or BOOT_CPU_MODE_EL2 in w0 if
 * booted in EL1 or EL2 respectively.
 */
el2_setup:
	msr	SPsel, #1			// We want to use SP_EL{1,2}
	mrs	x0, CurrentEL
	cmp	x0, #CurrentEL_EL2
	b.eq	1f
	ldr x0, =(SCTLR_EL1_RES1 | ENDIAN_SET_EL1)

	msr	sctlr_el1, x0
	mov	w0, #BOOT_CPU_MODE_EL1		// This cpu booted in EL1
	isb
	ret

1:	ldr x0, =(SCTLR_EL2_RES1 | ENDIAN_SET_EL2)
	msr	sctlr_el2, x0

	/*
	 * Check for VHE being present. For the rest of the EL2 setup,
	 * x2 being non-zero indicates that we do have VHE, and that the
	 * kernel is intended to run at EL2.
	 */
	mrs	x2, id_aa64mmfr1_el1
	ubfx	x2, x2, #ID_AA64MMFR1_VHE_SHIFT, #4

	/* Hyp configuration. */
	ldr x0, =HCR_HOST_NVHE_FLAGS
	cbz	x2, set_hcr
	ldr x0, =HCR_HOST_VHE_FLAGS

set_hcr:
	msr	hcr_el2, x0
	isb

	/*
	 * Allow Non-secure EL1 and EL0 to access physical timer and counter.
	 * This is not necessary for VHE, since the host kernel runs in EL2,
	 * and EL0 accesses are configured in the later stage of boot process.
	 * Note that when HCR_EL2.E2H == 1, CNTHCTL_EL2 has the same bit layout
	 * as CNTKCTL_EL1, and CNTKCTL_EL1 accessing instructions are redefined
	 * to access CNTHCTL_EL2. This allows the kernel designed to run at EL1
	 * to transparently mess with the EL0 bits via CNTKCTL_EL1 access in
	 * EL2.
	 */
	cbnz	x2, 1f
	mrs	x0, cnthctl_el2
	orr	x0, x0, #3			// Enable EL1 physical timers
	msr	cnthctl_el2, x0
1:
	msr	cntvoff_el2, xzr		// Clear virtual offset

	/* GICv3 system register access */
	mrs	x0, id_aa64pfr0_el1
	ubfx	x0, x0, #ID_AA64PFR0_GIC_SHIFT, #4
	cbz	x0, 3f

	mrs_s	x0, SYS_ICC_SRE_EL2
	orr	x0, x0, #ICC_SRE_EL2_SRE	// Set ICC_SRE_EL2.SRE==1
	orr	x0, x0, #ICC_SRE_EL2_ENABLE	// Set ICC_SRE_EL2.Enable==1
	msr_s	SYS_ICC_SRE_EL2, x0
	isb					// Make sure SRE is now set
	mrs_s	x0, SYS_ICC_SRE_EL2		// Read SRE back,
	tbz	x0, #0, 3f			// and check that it sticks
	msr_s	SYS_ICH_HCR_EL2, xzr		// Reset ICC_HCR_EL2 to defaults

3:

	/* Populate ID registers. */
	mrs	x0, midr_el1
	mrs	x1, mpidr_el1
	msr	vpidr_el2, x0
	msr	vmpidr_el2, x1

	msr	hstr_el2, xzr			// Disable CP15 traps to EL2

	/* EL2 debug */
	mrs	x1, id_aa64dfr0_el1
	sbfx	x0, x1, #ID_AA64DFR0_PMUVER_SHIFT, #4
	cmp	x0, #1
	b.lt	4f				// Skip if no PMU present
	mrs	x0, pmcr_el0			// Disable debug access traps
	ubfx	x0, x0, #11, #5			// to EL2 and allow access to
4:
	csel	x3, xzr, x0, lt			// all PMU counters from EL1

	/* Statistical profiling */
	ubfx	x0, x1, #ID_AA64DFR0_PMSVER_SHIFT, #4
	cbz	x0, 7f				// Skip if SPE not present
	cbnz	x2, 6f				// VHE?
	mrs_s	x4, SYS_PMBIDR_EL1		// If SPE available at EL2,
	and	x4, x4, #(1 << SYS_PMBIDR_EL1_P_SHIFT)
	cbnz	x4, 5f				// then permit sampling of physical
	mov	x4, #(1 << SYS_PMSCR_EL2_PCT_SHIFT | \
		      1 << SYS_PMSCR_EL2_PA_SHIFT)
	msr_s	SYS_PMSCR_EL2, x4		// addresses and physical counter
5:
	mov	x1, #(MDCR_EL2_E2PB_MASK << MDCR_EL2_E2PB_SHIFT)
	orr	x3, x3, x1			// If we don't have VHE, then
	b	7f				// use EL1&0 translation.
6:						// For VHE, use EL2 translation
	orr	x3, x3, #MDCR_EL2_TPMS		// and disable access from EL1
7:
	msr	mdcr_el2, x3			// Configure debug traps

	/* LORegions */
	mrs	x1, id_aa64mmfr1_el1
	ubfx	x0, x1, #ID_AA64MMFR1_LOR_SHIFT, 4
	cbz	x0, 1f
	msr_s	SYS_LORC_EL1, xzr
1:

	/* Stage-2 translation */
	msr	vttbr_el2, xzr

	cbz	x2, install_el2_stub

	mov	w0, #BOOT_CPU_MODE_EL2		// This CPU booted in EL2
	isb
	ret

install_el2_stub:
	/*
	 * When VHE is not in use, early init of EL2 and EL1 needs to be
	 * done here.
	 * When VHE _is_ in use, EL1 will not be used in the host and
	 * requires no configuration, and all non-hyp-specific EL2 setup
	 * will be done via the _EL1 system register aliases in __cpu_setup.
	 */
	ldr	x0, =(SCTLR_EL1_RES1 | ENDIAN_SET_EL1)
	msr	sctlr_el1, x0

	/* Coprocessor traps. */
	mov	x0, #0x33ff
	msr	cptr_el2, x0			// Disable copro. traps to EL2

	/* SVE register access */
	mrs	x1, id_aa64pfr0_el1
	ubfx	x1, x1, #ID_AA64PFR0_SVE_SHIFT, #4
	cbz	x1, 7f

	bic	x0, x0, #CPTR_EL2_TZ		// Also disable SVE traps
	msr	cptr_el2, x0			// Disable copro. traps to EL2
	isb
	mov	x1, #ZCR_ELx_LEN_MASK		// SVE: Enable full vector
	msr_s	SYS_ZCR_EL2, x1			// length for EL1.

	/* Hypervisor stub */
7:	adrp x0, __hyp_stub_vectors
	msr	vbar_el2, x0

	/* spsr */
	mov	x0, #(PSR_F_BIT | PSR_I_BIT | PSR_A_BIT | PSR_D_BIT | PSR_MODE_EL1h)
	msr	spsr_el2, x0
	msr	elr_el2, lr
	mov	w0, #BOOT_CPU_MODE_EL2		// This CPU booted in EL2

	eret

	nop
	nop
	nop

#endif /* !CONFIG_ARM64VT */

#ifdef CONFIG_SMP

ENTRY(secondary_startup)
	/*
     * Common entry point for secondary CPUs.
     */

	bl	el2_setup			// Drop to EL1, w0=cpu_boot_mode

	// Initialize the vector table
#ifndef CONFIG_SO3VIRT

	// Initialize the vector table
	ldr	x0, =__vectors

#ifdef CONFIG_ARM64VT
	msr vbar_el2, x0
#else
	msr vbar_el1, x0
#endif /* !CONFIG_ARM64VT */

#endif /* !CONFIG_SO3VIRT */

	// Enable FP/ASIMD
	mov	x0, #3 << 20
	msr	cpacr_el1, x0

    // Up to here, a stack should be initialized
    adr 	x4, __secondary_data

	ldr		x5, [x4]
	ldr		x19, [x4, #8]

	// Nice trick to retrieve secondary_data field values (pgdir, stack)
	// x5 is a virtual addresses at same location than x4 containing the physical location.
	// So x10 will contain the offset
	sub		x10, x4, x5

	// Get secondary_data.pgdir (x19 is virtual, x0 is physical)
	ldr		x0, [x10, x19]

	// Get the stack pointer (virt. address)
	add		x19, x19, #8
	ldr		x8, [x10, x19]
	mov		sp, x8

	// Convert to physical address
	add		sp, sp, x10

	bl		mmu_configure

	ldr		x8, [x19]
	mov		sp, x8

  	// Keep executing in the kernel space

  	// Store the virtual address which will be used to continue
    // execution after the MMU enabled.
  	ldr		x19, .LC_virt_secondary_entry

  	blr		x19

	nop
	nop
	nop

    .type   __secondary_data, %object

__secondary_data:
    .quad   .
    .quad   secondary_data

.LC_virt_secondary_entry:
  .quad secondary_start_kernel

#endif /* CONFIG_SMP */

.ltorg

__fdt_addr:
  .quad 	0

.align 13

// Before MMU is enabled, we cannot refer to the normal stack as declared in the linker script
temp_stack_bottom:
	.space 4096
temp_stack:

.LCvirt_entry:
  .quad __kernel_main

