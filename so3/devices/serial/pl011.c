/*
 * Copyright (C) 2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <types.h>
#include <heap.h>

#include <process.h>
#include <signal.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/serial.h>
#include <device/irq.h>

#include <device/arch/pl011.h>

#include <mach/uart.h>

#include <asm/io.h>                 /* ioread/iowrite macros */

#include <printk.h>

#define AMBA_ISR_PASS_LIMIT	256
#define SERIAL_BUFFER_SIZE	80

static volatile char serial_buffer[SERIAL_BUFFER_SIZE];
static volatile uint32_t prod=0, cons=0;

extern mutex_t read_lock;

static dev_t pl011_dev =
{
  .base = UART_BASE,
};

static int pl011_put_byte(char c) {

	while ((ioread16(pl011_dev.base + UART01x_FR) & UART01x_FR_TXFF)) ;

	iowrite16(pl011_dev.base + UART01x_DR, c);

	return 1;
}


static char pl011_get_byte(bool polling) {
	char tmp;

	if (polling) {
		
		/* Poll while nothing available */
		while (ioread8(pl011_dev.base + UART01x_FR) & UART01x_FR_RXFE) ;

		return ioread16(pl011_dev.base + UART01x_DR);
	} else {
		while (prod == cons) {

			schedule();

			__asm("dsb");
			__asm("wfi");
		}

		tmp = serial_buffer[cons];
		cons = (cons + 1) % SERIAL_BUFFER_SIZE;

		return tmp;

	}

	/* Makes gcc happy */
	return 0;
}

/*
 * The interrupt routine consists in reading the char which has been typed by the user.
 * Characters are stored in the seial buffer.
 * To know if the current thread is doing a read on the UART, we test the read_lock mutex
 * to decide what to do in case of a ctrl+C key.
 * If the mutex is taken by the thread, it means the thread acquired a lock and we do not
 * propagate the SIGINT signal in the interrupt routine to avoid pre-matured exit of the process
 * holding the lock. The processing of SIGINT will be done by the reading function in the HAL.
 */
static irq_return_t pl011_int(int irq, void *dummy)
{
	unsigned int status, pass_counter = AMBA_ISR_PASS_LIMIT;

	status = ioread16(pl011_dev.base + UART011_MIS);
	if (status) {
		do {
			iowrite16(pl011_dev.base + UART011_ICR, status & ~(UART011_TXIS|UART011_RTIS | UART011_RXIS));

			if (status & (UART011_RTIS|UART011_RXIS)) {
				serial_buffer[prod] = ioread16(pl011_dev.base + UART01x_DR);

				/* Check for SIGINT to be raised on Ctrl^C */
				if (serial_buffer[prod] == 3) {

					pl011_put_byte('^');
					pl011_put_byte('C');
					pl011_put_byte('\n');
					prod--; /* Already sent out to the serial interface */

#ifdef CONFIG_IPC_SIGNAL
					if (current()->pcb != NULL)
						do_kill(current()->pcb->pid, SIGINT);
#endif
				}

				prod = (prod + 1) % SERIAL_BUFFER_SIZE;
			}
			if (pass_counter-- == 0)
				break;

			status = ioread16(pl011_dev.base + UART011_MIS);

		} while (status != 0);
	}

	return IRQ_COMPLETED;
}


static int pl011_init(dev_t *dev) {

	/* Init pl011 UART */

	memcpy(&pl011_dev, dev, sizeof(dev_t));

	serial_ops.put_byte = pl011_put_byte;
	serial_ops.get_byte = pl011_get_byte;

	serial_ops.dev = dev;

	/* Bind ISR into interrupt controller */
	irq_bind(dev->irq_nr, pl011_int, NULL, NULL);

	/* Enable interrupt (IRQ controller) */
	iowrite16(pl011_dev.base + UART011_IMSC, UART011_RXIM | UART011_RTIM);

	irq_ops.irq_enable(dev->irq_nr);

	return 0;
}

void __ll_put_byte(char c) {

	pl011_put_byte(c);

}

REGISTER_DRIVER_POSTCORE("serial,pl011", pl011_init);
