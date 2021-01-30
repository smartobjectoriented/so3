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

/*
 * Low-level ARM-specific setup
 */

#include <memory.h>

#include <device/driver.h>
#include <device/irq.h>

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/setup.h>

extern unsigned char __irq_stack_start[];

/* To keep the original CPU ID so that we can avoid
 * undesired activities running on another CPU.
 */
uint32_t origin_cpu;

/*
 * Only 3 32-bit fields are sufficient (see exception.S)
 */
struct stack {
	u32 irq[3];
	u32 abt[3];
	u32 und[3];
	u32 fiq[3];
} ____cacheline_aligned;

struct stack stacks;

/*
 * Setup exceptions stacks for all modes except SVC and USR
 */
void setup_exception_stacks(void) {
	struct stack *stk = &stacks;

	/* Need to set the CPU in the different modes and back to SVC at the end */
	__asm__ (
		"msr	cpsr_c, %1\n\t"
		"add	r14, %0, %2\n\t"
		"mov	sp, r14\n\t"
		"msr	cpsr_c, %3\n\t"
		"add	r14, %0, %4\n\t"
		"mov	sp, r14\n\t"
		"msr	cpsr_c, %5\n\t"
		"add	r14, %0, %6\n\t"
		"mov	sp, r14\n\t"
		"msr	cpsr_c, %7\n\t"
		"add	r14, %0, %8\n\t"
		"mov	sp, r14\n\t"
		"msr	cpsr_c, %9"
		    :
		    : "r" (stk),
		      "I" (PSR_F_BIT | PSR_I_BIT | PSR_IRQ_MODE), "I" (offsetof(struct stack, irq[0])),
		      "I" (PSR_F_BIT | PSR_I_BIT | PSR_ABT_MODE), "I" (offsetof(struct stack, abt[0])),
		      "I" (PSR_F_BIT | PSR_I_BIT | PSR_UND_MODE), "I" (offsetof(struct stack, und[0])),
		      "I" (PSR_F_BIT | PSR_I_BIT | PSR_FIQ_MODE), "I" (offsetof(struct stack, fiq[0])),
		      "I" (PSR_F_BIT | PSR_I_BIT | PSR_SVC_MODE)
		    : "r14");

}

void arm_init_domains(void)
{
	u32 reg;

	reg = get_dacr();
	/*
	* Set DOMAIN to client access so that all permissions
	* set in pagetables are validated by the mmu.
	*/
	reg &= ~DOMAIN_MASK;
	reg |= DOMAIN_MANAGER;
	set_dacr(reg);
}

/**
 * Enabling VFP coprocessor.
 * Currenty, we do not manage vfp context switch
 */
#warning vfp context switch to be implemented...
void vfp_enable(void)
{
	u32 access;

	access = get_copro_access();

	/*
	 * Enable full access to VFP (cp10 and cp11)
	 */
	set_copro_access(access | CPACC_FULL(10) | CPACC_FULL(11));

	__enable_vfp();
}

/**
 * Low-level initialization before the main boostrap process.
 */
void setup_arch(void) {
	int offset;

	/* Clear BSS - DO NOT ASSIGN VALUES TO NON-INITIALIZED VARIABLES BEFORE THIS POINT.*/
	clear_bss();

	/* Retrieve information about the main memory (RAM) */

	/* Access to device tree */
	offset = get_mem_info((void *) __fdt_addr, &mem_info);
	if (offset >= 0)
		DBG("Found %d MB of RAM at 0x%08X\n", mem_info.size / SZ_1M, mem_info.phys_base);

	/* Original boot CPU identification to prevent undesired activities on another CPU . */
	origin_cpu = smp_processor_id();

	/* Set up the different stacks according to CPU mode */
	setup_exception_stacks();

	/* Keep a reference to the 1st-level system page table */
#ifdef CONFIG_MMU
	__sys_l1pgtable = (unsigned int *) (CONFIG_RAM_BASE + TTB_L1_SYS_OFFSET);
#endif

#if 0 /* At the moment, we do not handle security in user space */
	/* Change the domain access controller to enable kernel protection against user access */
	set_domain(0xfffffffd);
#endif
	vfp_enable();

	lprintk("%s: CPU control register (CR) = %x\n", __func__, get_cr());

	/* A low-level UART should be initialized here so that subsystems initialization (like MMC) can already print out logs ... */

}
