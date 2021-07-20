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


#include <device/arch/riscv_timer.h>

void (*isr_handler[256])(void);

/* Attribute interrupt for gcc is used to avoid writing assembly ABI code. It auto generates the handler
 * to save and restore registers correctly */
void handle_trap(void) __attribute((interrupt)) ;

void handle_trap() {

	printk("Got to trap handler, yay :D\n");

	u64 mcause = csr_read(CSR_CAUSE);
	u64 value = CAUSE_IRQ_FLAG;

	/* Interrupt source is on the other bits of mcause reg.
	 * Note: Wierd bug won't let the constant be used directly... */
	u64 trap_source = mcause & ~value;

	/* TODO _NMR_ remove */
	isr_handler[7] = timer_isr;

	/* If reg MSB is 1, it is an interrupt */
	if (mcause & CAUSE_IRQ_FLAG) {

		/* Call irq handler of correct cause */
		isr_handler[trap_source]();
	}
	/* Else it is an exception */
	else {

		/* Call exception handler of correct cause */
#if 0
		exception_handler[trap_source]();
#endif

	}
}
