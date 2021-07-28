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

/*
 * IRQs are handled in supervisor mode
 * Exceptions are handled in machine mode
 */

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


/* Attribute interrupt for gcc is used to avoid writing assembly ABI code. It auto generates the handler
 * to save and restore registers that are modified correctly */
void handle_mmode_trap(void) __attribute((interrupt)) ;
void handle_mmode_trap() {

	u64 mcause = csr_read(CSR_MCAUSE);

	/* Interrupt source is on the other bits of mcause reg.
	 * max trap source is 15 yet.. Mask 0xfff is more than enough */
	u64 trap_source = mcause & 0xfff;

	/* If reg MSB is 1, it is an interrupt */
	if (mcause & CAUSE_IRQ_FLAG) {

		/* Sould not happen because IRQs are forwarded to supervisor mode.
		 * Kernel panics to know what happend */
		printk("Irqs should be forwarded to supervisor mode.\n"
			   "Got IRQ is machine mode : No is %d\n", trap_source);
		kernel_panic();

	}
	/* Else it is an exception */
	else {

		/* Call exception handler of correct cause */
		if (exception_handler_registred[trap_source]) {
			exception_handlers[trap_source]();
		}
		else {
			printk("Got Environement call (ECALL) or Breakpoint (EBREAK) "
					": No is %d\n"
					"There is no implementation for this case yet !\n", trap_source);
			kernel_panic();
		}

	}
}

/* Attribute interrupt for gcc is used to avoid writing assembly ABI code. It auto generates the handler
 * to save and restore registers correctly */
void handle_smode_trap(void) __attribute((interrupt)) ;
void handle_smode_trap() {

	u64 mcause = csr_read(CSR_SCAUSE);

	/* Interrupt source is on the other bits of mcause reg.
	 * max trap source is 15 yet.. Mask 0xfff is more than enough */
	u64 trap_source = mcause & 0xfff;

	/* If reg MSB is 1, it is an interrupt */
	if (mcause & CAUSE_IRQ_FLAG) {

		switch(trap_source) {

		/* Goes to timer ISR */
		case RV_IRQ_TIMER:

			__in_interrupt = true;

			timer_isr(trap_source, NULL);

#if 0 /* Issue is active to define if the check is really relevant */
			/* TODO update function with registers saved instead of the current ones */
			if (local_irq_is_disabled())
				do_softirq();
#endif
			/* Perform the softirqs. Since this function is accessed in machine mode with interrupts
			 * disabled there should be no need to check flags */
			do_softirq();

			break;

		/* Will be handled by the PLIC */
		case RV_IRQ_EXT:

			/* This function is located in irq.c and is the generic way to handle an
			 * irq. */
			irq_handle(NULL);

			break;
		default:
			/* Sould not happen since last type possible here is for SOFT_IRQs and SO3
			 * doesn't use softirqs as real hardware irqs. */
			printk("Ignoring unkown IRQ source : No is %d\n", trap_source);
		}
	}
	/* Else it is an exception */
	else {

		/* Sould not happen because exceptions are handled in machine mode.
		 * Kernel panics to know what happend */
		printk("Exceptions should be handled in machine mode.\n"
			   "Got Exception in supervisor mode : No is %d\n", trap_source);
		kernel_panic();

	}
}

/* Inits all exception handlers */
void init_trap() {
	int i;

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

