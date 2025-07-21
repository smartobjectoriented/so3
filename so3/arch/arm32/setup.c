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
#include <log.h>

#include <device/driver.h>
#include <device/irq.h>

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/setup.h>
#include <asm/vfp.h>

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

struct stack stacks[CONFIG_NR_CPUS];

/*
 * Setup exceptions stacks for all modes except SVC and USR
 */
void setup_exception_stacks(void)
{
	struct stack *stk = &stacks[smp_processor_id()];

	/* Need to set the CPU in the different modes and back to SVC at the end */
	__asm__("msr	cpsr_c, %1\n\t"
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
		: "r"(stk), "I"(PSR_F_BIT | PSR_I_BIT | PSR_IRQ_MODE), "I"(offsetof(struct stack, irq[0])),
		  "I"(PSR_F_BIT | PSR_I_BIT | PSR_ABT_MODE), "I"(offsetof(struct stack, abt[0])),
		  "I"(PSR_F_BIT | PSR_I_BIT | PSR_UND_MODE), "I"(offsetof(struct stack, und[0])),
		  "I"(PSR_F_BIT | PSR_I_BIT | PSR_FIQ_MODE), "I"(offsetof(struct stack, fiq[0])),
		  "I"(PSR_F_BIT | PSR_I_BIT | PSR_SVC_MODE)
		: "r14");
}

void arm_init_domains(void)
{
	u32 reg;

	reg = get_dacr();

	/*
	* Set DOMAIN to manager access so that all access are permitted.
	*/

	reg &= ~DOMAIN_MASK;
	reg |= DOMAIN_MANAGER;
	set_dacr(reg);
}

void cpu_init(void)
{
	/* Original boot CPU identification to prevent undesired activities on another CPU . */
	origin_cpu = smp_processor_id();

	/* Set up the different stacks according to CPU mode */
	setup_exception_stacks();
}

/**
 * Low-level initialization before the main boostrap process.
 */
void setup_arch(void)
{
	/* Original boot CPU identification to prevent undesired activities on another CPU . */
	origin_cpu = smp_processor_id();

	/* Set up the different stacks according to CPU mode */
	setup_exception_stacks();

#if 0 /* At the moment, we do not handle security in user space */
	/* Change the domain access controller to enable kernel protection against user access */
	set_domain(0xfffffffd);
#endif
	vfp_enable();

	LOG_DEBUG("%s: CPU control register (CR) = %x\n", __func__, get_cr());

	/* A low-level UART should be initialized here so that subsystems initialization (like MMC) can already print out logs ... */
}
