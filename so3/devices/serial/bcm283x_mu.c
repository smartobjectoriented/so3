/*
 * Copyright (C) 2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <heap.h>
#include <limits.h>

#include <device/device.h>
#include <device/driver.h>

#include <device/serial.h>

#include <device/arch/bcm283x_mu.h>
#include <mach/uart.h>

#include <asm/io.h>                 /* ioread/iowrite macros */

static dev_t bcm283x_mu_dev =
{
  .base = UART_BASE,
};

static int bcm283x_mu_put_byte(char c)
{
	bcm283x_mu_t *bcm283x_mu = (bcm283x_mu_t *) bcm283x_mu_dev.base;

	/* Wait until there is space in the FIFO */
	while (!(ioread32(&bcm283x_mu->lsr) & UART_LSR_TX_READY)) ;

	/* Send the character */
	iowrite32(&bcm283x_mu->io, (uint32_t) c);

	if (c == '\n') {
		while (!(ioread32(&bcm283x_mu->lsr) & UART_LSR_TX_READY)) ;
		iowrite8(&bcm283x_mu->io, '\r');	/* Carriage return */
	}

	return 0;
}

void __ll_put_byte(char c) {
	bcm283x_mu_put_byte(c);
}

static char bcm283x_mu_get_byte(bool polling)
{	 
	bcm283x_mu_t *bcm283x_mu = (bcm283x_mu_t *) bcm283x_mu_dev.base;

	while (!(ioread32(&bcm283x_mu->lsr) & UART_LSR_RX_READY)) ;

	return (char) ioread32(&bcm283x_mu->io);
}

static int bcm283x_mu_init(dev_t *dev)
{

	/* Pins multiplexing skipped here for simplicity (done by bootloader) */
	/* Clocks init skipped here for simplicity (done by bootloader) */

	/* Initialize UART controller */
	memcpy(&bcm283x_mu_dev, dev, sizeof(dev_t));

	serial_ops.put_byte = bcm283x_mu_put_byte;
	serial_ops.get_byte = bcm283x_mu_get_byte;

	serial_ops.dev = dev;

	return 0;

}

REGISTER_DRIVER_POSTCORE("serial,bcm283x-mu", bcm283x_mu_init);
