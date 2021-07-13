/*
 * Copyright (C) 2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <heap.h>
#include <limits.h>

#include <device/device.h>
#include <device/driver.h>

#include <device/serial.h>

#include <device/arch/ns16550.h>
#include <mach/uart.h>

#include <asm/io.h>                 /* ioread/iowrite macros */

static dev_t ns16550_dev =
{
  .base = UART_BASE,
};

static void *base_addr = (void*)UART_BASE;

static int baudrate_div_calc(int baudrate)
{
	int divider;

	/* Internal clock is at 24 MHz (UART_CLK_FREQ) */
	divider = (UART_CLK_FREQ + (baudrate * 8)) / (baudrate * 16);

	/* Divider cannot be encoded on 16 bits */
	if (divider > UINT16_MAX)
		BUG();

	return divider;

}

static int ns16550_put_byte(char c)
{

	while ((ioread8(base_addr + UART_LSR_REG_OFFSET) & UART_LSR_THRE) == 0);

	if (c == '\n') {
		iowrite8(base_addr + UART_RBR_REG_OFFSET, '\n');	/* Line Feed */

		while ((ioread8(base_addr + UART_LSR_REG_OFFSET) & UART_LSR_THRE) == 0);

		iowrite8(base_addr + UART_RBR_REG_OFFSET, '\r');	/* Carriage return */

	} else {
		/* Output character */
		iowrite8(base_addr + UART_RBR_REG_OFFSET, c); /* Transmit char */
	}

	return 0;
}

void __ll_put_byte(char c) {
	ns16550_put_byte(c);
}

static char ns16550_get_byte(bool polling)
{
	while ((ioread8(base_addr + UART_LSR_REG_OFFSET) & UART_LSR_DR) == 0);

	return ioread8(base_addr + UART_RBR_REG_OFFSET);
}

static int ns16550_init(dev_t *dev)
{
	int baudrate = UART_BAUDRATE;
	int divider;

	/* Pins multiplexing skipped here for simplicity (done by bootloader) */
	/* Clocks init skipped here for simplicity (done by bootloader) */

	/* Initialize UART controller */
	memcpy(&ns16550_dev, dev, sizeof(dev_t));

	serial_ops.put_byte = ns16550_put_byte;
	serial_ops.get_byte = ns16550_get_byte;

	/* Ensure all interrupts are disabled */
	iowrite8(base_addr + UART_IER_REG_OFFSET, 0);

	/* Put the UART in 'Configuration mode A' to allow baudrate configuration */
	iowrite8(base_addr + UART_LCR_REG_OFFSET, UART_LCR_DLAB);

	/* Configure baudrate */
	divider = baudrate_div_calc(baudrate);
	if (divider < 0) {
		return -1;
	}

	iowrite8(base_addr + UART_DLL_REG_OFFSET, divider & 0xFF);
	iowrite8(base_addr + UART_DLH_REG_OFFSET, (divider >> 8) & 0xFF);

	/* 8N1 standard configuration */
	iowrite8(base_addr + UART_LCR_REG_OFFSET, UART_PARITY_DIS | UART_1_STOP | UART_8BITS );

	/* Force RTS and DTR lines */
	iowrite8(base_addr + UART_MCR_REG_OFFSET, UART_RTS | UART_DTR);

	return 0;

}

REGISTER_DRIVER_POSTCORE("serial,ns16550", ns16550_init);
