/*
 * Copyright (C) 2016,2017 Daniel Rossier <daniel.rossier@soo.tech>
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

#include <common.h>
#include <percpu.h>
#include <console.h>

#include <asm/processor.h>
#include <asm/backtrace.h>

static const char *processor_modes[] = {
  "USER_26", "FIQ_26" , "IRQ_26" , "SVC_26" , "UK4_26" , "UK5_26" , "UK6_26" , "UK7_26" ,
  "UK8_26" , "UK9_26" , "UK10_26", "UK11_26", "UK12_26", "UK13_26", "UK14_26", "UK15_26",
  "USER_32", "FIQ_32" , "IRQ_32" , "SVC_32" , "UK4_32" , "UK5_32" , "MON_32" , "ABT_32" ,
  "UK8_32" , "UK9_32" , "HYP_32", "UND_32" , "UK12_32", "UK13_32", "UK14_32", "SYS_32"
};

void show_registers(struct cpu_regs *regs);
extern void __backtrace(void);

void show_backtrace(ulong sp, ulong lr, ulong pc)
{
    __backtrace();
}

void show_backtrace_regs(struct cpu_regs *regs)
{
    show_registers(regs);
    __backtrace();
}

void show_registers(struct cpu_regs *regs)
{
	unsigned long flags = regs->psr;

	printk("CPU: %d\n", smp_processor_id());

	printk("PC is at %08lx\n", (unsigned long) regs->pc);
	printk("LR is at %08lx\n", (unsigned long) regs->lr);
	printk("pc : [<%08lx>]    lr : [<%08lx>]    \n"
	       "sp : %08lx  ip : %08lx  fp : %08lx\n",
		(unsigned long) regs->pc,
		(unsigned long) regs->lr, (unsigned long) regs->sp,
		(unsigned long) regs->ip, (unsigned long) regs->fp);
	printk("r10: %08lx  r9 : %08lx  r8 : %08lx\n",
		(unsigned long) regs->r10, (unsigned long) regs->r9,
		(unsigned long) regs->r8);
	printk("r7 : %08lx  r6 : %08lx  r5 : %08lx  r4 : %08lx\n",
		(unsigned long) regs->r7, (unsigned long) regs->r6,
		(unsigned long) regs->r5, (unsigned long) regs->r4);
	printk("r3 : %08lx  r2 : %08lx  r1 : %08lx  r0 : %08lx\n",
		(unsigned long) regs->r3, (unsigned long) regs->r2,
		(unsigned long) regs->r1, (unsigned long) regs->r0);
	printk("Flags: %c%c%c%c",
		flags & PSR_N_BIT ? 'N' : 'n',
		flags & PSR_Z_BIT ? 'Z' : 'z',
		flags & PSR_C_BIT ? 'C' : 'c',
		flags & PSR_V_BIT ? 'V' : 'v');
	printk("  IRQs o%s  FIQs o%s  Mode %s%s\n",
		interrupts_enabled(regs) ? "n" : "ff",
		fast_interrupts_enabled(regs) ? "n" : "ff",
		processor_modes[processor_mode(regs)],
		"ARM");

	{
		unsigned int ctrl, transbase, dac;
		  __asm__ (
		"	mrc p15, 0, %0, c1, c0\n"
		"	mrc p15, 0, %1, c2, c0\n"
		"	mrc p15, 0, %2, c3, c0\n"
		: "=r" (ctrl), "=r" (transbase), "=r" (dac));
		printk("Control: %04X  Table: %08X  DAC: %08X\n",
		  	ctrl, transbase, dac);
	}

}

void dump_stack(void)
{
	__backtrace();
}

void dump_execution_state(void)
{
#if 0
    struct cpu_user_regs regs;

	register unsigned int r0 __asm__("r0");
	register unsigned int r1 __asm__("r1");
	register unsigned int r2 __asm__("r2");
	register unsigned int r3 __asm__("r3");
	register unsigned int r4 __asm__("r4");
	register unsigned int r5 __asm__("r5");
	register unsigned int r6 __asm__("r6");
	register unsigned int r7 __asm__("r7");
	register unsigned int r8 __asm__("r8");
	register unsigned int r9 __asm__("r9");
	register unsigned int r10 __asm__("r10");
	register unsigned int r11 __asm__("r11");
	register unsigned int r12 __asm__("r12");
	register unsigned int r13 __asm__("r13");
	register unsigned int r14 __asm__("r14");
	register unsigned int r15;

	asm("mov %0, pc":"=r"(r15));

	regs.r0 = r0;
	regs.r1 = r1;
	regs.r2 = r2;
	regs.r3 = r3;
	regs.r4 = r4;
	regs.r5 = r5;
	regs.r6 = r6;
	regs.r7 = r7;
	regs.r8 = r8;
	regs.r9 = r9;
	regs.r10 = r10;
	regs.r11 = r11;
	regs.r12 = r12;
	regs.r13 = r13;
	regs.r14 = r14;
	regs.r15 = r15;

	__asm__ __volatile__("mrs %0, cpsr " : "=r" (regs.psr) : : "memory", "cc");

    show_registers(&regs);
#endif
}


void dump_all_execution_state(void)
{
    ulong sp;
    ulong lr;

    dump_execution_state();
    sp = (ulong)__builtin_frame_address(0);
    lr = (ulong)__builtin_return_address(0);

    show_backtrace(sp, lr, lr);
}

void vcpu_show_execution_state(void)
{
    printk("*** Dumping current execution state ***\n");


    dump_execution_state();
}

