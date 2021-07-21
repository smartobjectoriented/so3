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

#include <common.h>
#include <asm/csr.h>
#include <device/irq.h>
#include <softirq.h>

#include <device/arch/riscv_timer.h>

extern void irq_handle(cpu_regs_t *regs);


/* There are maximum 16 standard irq sources in register mcause. Can be extended with custom
 * irq if needed. */
static irq_handler_t isr_handlers[16];

/* Attribute interrupt for gcc is used to avoid writing assembly ABI code. It auto generates the handler
 * to save and restore registers correctly */
void handle_trap(void) __attribute((interrupt)) ;

void handle_trap() {

	u64 mcause = csr_read(CSR_CAUSE);
	u64 flag = CAUSE_IRQ_FLAG;

	/* Interrupt source is on the other bits of mcause reg.
	 * Note: Wierd bug won't let the constant be used directly... */
	u64 trap_source = mcause & ~flag;

	/* If reg MSB is 1, it is an interrupt */
	if (mcause & CAUSE_IRQ_FLAG) {

		printk("Got IRQ in trap handler :D\n");

		switch(trap_source) {

		/* Goes to timer ISR */
		case RV_IRQ_TIMER:

			__in_interrupt = true;

			isr_handlers[trap_source](0, NULL);

			/* Now perform the softirq processing if allowed, i.e. if previously we have been upcalled
			 * with IRQs on. _NMR_ TODO update function with registers saved instead of the current ones */
			if (local_irq_is_disabled())
				do_softirq();

			break;

		/* Will be handled by the PLIC */
		case RV_IRQ_EXT:

			/* This function is located in irq.c and is the generic way to handle an
			 * irq. */
			irq_handle(NULL);

			break;
		default:
			printk("Ignoring unkown IRQ source : No is %d\n", trap_source);
		}
	}
	/* Else it is an exception */
	else {

		printk("Got exception in trap handler cause = %d\n", trap_source);

		/* Call exception handler of correct cause */
#if 0
		exception_handler[trap_source]();
#endif

	}
}

void register_isr_for_trap(int no_irq, irq_handler_t handler) {
	isr_handlers[no_irq] = handler;
}


