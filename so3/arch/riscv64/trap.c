/*
 * Copyright (C) 2021 Nicolas Müller <nicolas.muller1@heig-vd.ch>
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

#if 1
#define DEBUG
#endif

#include <common.h>
#include <asm/csr.h>
#include <device/irq.h>
#include <softirq.h>

#include <device/arch/riscv_timer.h>
#include <asm/trap.h>
#include <asm/fault.h>

/* Define exception numbers */
#define INSTR_ADDR_MISALIGNED	0
#define INSTR_ACCESS_FAULT		1
#define ILLEGAL_INSTR			2
#define LOAD_ADDR_MISALIGNED	4
#define LOAD_ACCESS_FAULT		5
#define STORE_ADDR_MISALIGNED	6
#define STORE_ACCESS_FAULT		7
#define INSTR_PAGE_FAULT		12
#define LOAD_PAGE_FAULT			13
#define STORE_PAGE_FAULT		15

#define EXCEPTION_COUNT 	16

extern void irq_handle(cpu_regs_t *regs);
extern irq_return_t timer_isr(int irq, void *dummy);

/* Store exceptions.. 16 is the number of standard exceptions */
static exception_handler_t exception_handlers[EXCEPTION_COUNT];
static bool exception_handler_registred[EXCEPTION_COUNT];

void trap_dump_regs(cpu_regs_t *regs);

/* Once in a trap, registers are stored in the trap frame */
cpu_regs_t trap_frame;

/* Attribute interrupt for gcc is used to avoid writing assembly ABI code. It auto generates the handler
 * to save and restore registers correctly */
u64 handle_trap(u64 epc, u64 tval, u64 cause, u64 status, cpu_regs_t *regs) {

	/* Interrupt source is on the other bits of mcause reg.
	 * 0xfff is more than enough. There are 15 exceptions and 15 irqs in RISC-V for now */
	u64 trap_source = cause & 0xfff;

	/* If reg MSB is 1, it is an interrupt */
	if (cause & CAUSE_IRQ_FLAG) {

		switch(trap_source) {

		/* Goes to timer ISR */
		case RV_IRQ_TIMER:

			__in_interrupt = true;
			timer_isr(trap_source, NULL);

			/* Perform the softirqs if allowed */
			if (!irqs_disabled_flags(regs))
				do_softirq();

			break;
		/* Will be handled by the PLIC */
		case RV_IRQ_EXT:

			/* This function is located in irq.c and is the generic way to handle an
			 * irq. */
			irq_handle(regs);

			break;
		default:
			/* Sould not happen since last type possible here is for SOFT_IRQs and SO3
			 * doesn't use softirqs as real hardware irqs. */
			printk("Ignoring unkown IRQ source : No is %d\n", trap_source);
		}
	}
	/* Else it is an exception */
	else {

		/* Call exception handler of correct cause */
		if (exception_handler_registred[trap_source]) {
			printk("### Got exception at :\n"
				   "### instr addr:  %#16x\n"
				   "### status reg:  %#16x\n"
				   "### tval:        %#16x\n", epc, status, tval);
#ifdef DEBUG
			trap_dump_regs(regs);
#endif
			exception_handlers[trap_source]();
		}
		else {
			printk("Got Environement call (ECALL) or Breakpoint (EBREAK) "
					": No is %d\n"
					"There is no implementation for this case yet !\n", trap_source);
			kernel_panic();
		}

	}

	/* Value is used by low level trap handler to return at old pc */
	return epc;
}

void init_trap() {

	int i;

	/* Set trap frame into mscratch to have it entering the handler */
	csr_write(CSR_MSCRATCH, &trap_frame);

	/* Set all exception handler*/
	for (i = 0; i < EXCEPTION_COUNT; i++) {
		switch (i) {
		case INSTR_ADDR_MISALIGNED	:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __instr_addr_misalignment;
			break;
		case INSTR_ACCESS_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __instr_access_fault;
			break;
		case ILLEGAL_INSTR			:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __illegal_instr;
			break;
		case LOAD_ADDR_MISALIGNED	:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __load_addr_misalignement;
			break;
		case LOAD_ACCESS_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __load_access_fault;
			break;
		case STORE_ADDR_MISALIGNED	:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __store_AMO_addr_misaligned;
			break;
		case STORE_ACCESS_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __store_AMO_access_fault;
			break;
		case INSTR_PAGE_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __instr_page_fault;
			break;
		case LOAD_PAGE_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __load_page_fault;
			break;
		case STORE_PAGE_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __store_AMO_page_fault;
			break;
		default:
			exception_handler_registred[i] = false;
			exception_handlers[i] = NULL;
			break;
		}
	}
}

void trap_dump_regs(cpu_regs_t *regs) {

	printk("ra:  %#016x\n", regs->ra);
	printk("sp:  %#016x\n", regs->sp);
	printk("gp:  %#016x\n", regs->gp);
	printk("tp:  %#016x\n", regs->tp);
	printk("t0:  %#016x\n", regs->t0);
	printk("t1:  %#016x\n", regs->t1);
	printk("t2:  %#016x\n", regs->t2);
	printk("fp:  %#016x\n", regs->fp);
	printk("s1:  %#016x\n", regs->s1);
	printk("a0:  %#016x\n", regs->a0);
	printk("a1:  %#016x\n", regs->a1);
	printk("a2:  %#016x\n", regs->a2);
	printk("a3:  %#016x\n", regs->a3);
	printk("a4:  %#016x\n", regs->a4);
	printk("a5:  %#016x\n", regs->a5);
	printk("a6:  %#016x\n", regs->a6);
	printk("a7:  %#016x\n", regs->a7);
	printk("s2:  %#016x\n", regs->s2);
	printk("s3:  %#016x\n", regs->s3);
	printk("s4:  %#016x\n", regs->s4);
	printk("s5:  %#016x\n", regs->s5);
	printk("s6:  %#016x\n", regs->s6);
	printk("s7:  %#016x\n", regs->s7);
	printk("s8:  %#016x\n", regs->s8);
	printk("s9:  %#016x\n", regs->s9);
	printk("s10: %#016x\n", regs->s10);
	printk("s11: %#016x\n", regs->s11);
	printk("t3:  %#016x\n", regs->t3);
	printk("t4:  %#016x\n", regs->t4);
	printk("t5:  %#016x\n", regs->t5);
	printk("t6:  %#016x\n", regs->t6);
}

