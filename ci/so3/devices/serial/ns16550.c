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
#include <memory.h>

#include <device/device.h>
#include <device/driver.h>

#include <device/serial.h>

#include <device/arch/ns16550.h>

#include <asm/io.h>                 /* ioread/iowrite macros */

typedef struct {
	irq_def_t irq_def;

	ns16550_t *io;

} ns16550_dev_t;

static ns16550_dev_t ns16550_dev =
{
  .io = (ns16550_t *) CONFIG_UART_LL_PADDR,
};

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

	while ((ioread32(&ns16550_dev.io->lsr) & UART_LSR_THRE) == 0) ;

	if (c == '\n') {
		iowrite8(&ns16550_dev.io->rbr, '\n');	/* Line Feed */

		while ((ioread32(&ns16550_dev.io->lsr) & UART_LSR_THRE) == 0) ;

		iowrite8(&ns16550_dev.io->rbr, '\r');	/* Carriage return */

	} else {
		/* Output character */
		
		iowrite8(&ns16550_dev.io->rbr, c); /* Transmit char */
	}

	return 0;
}

void __ll_put_byte(char c) {
	ns16550_put_byte(c);
}

static char ns16550_get_byte(bool polling)
{	 
	while ((ioread32(&ns16550_dev.io->lsr) & UART_LSR_DR) == 0);

	return ioread32(&ns16550_dev.io->rbr);
}

static int ns16550_init(dev_t *dev, int fdt_offset)
{
	int baudrate = UART_BAUDRATE;
	int divider;
	const struct fdt_property *prop;
	int prop_len;

	/* Pins multiplexing skipped here for simplicity (done by bootloader) */
	/* Clocks init skipped here for simplicity (done by bootloader) */

	/* Initialize UART controller */
	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);

	BUG_ON(prop_len != 2 * sizeof(unsigned long));

#ifdef CONFIG_ARCH_ARM32
	ns16550_dev.io = (ns16550_t *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]), fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
#else
	ns16550_dev.io = (ns16550_t *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]), fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));
#endif

	serial_ops.put_byte = ns16550_put_byte;
	serial_ops.get_byte = ns16550_get_byte;

	/* Ensure all interrupts are disabled */
	iowrite32(&ns16550_dev.io->ier, 0);

	/* Put the UART in 'Configuration mode A' to allow baudrate configuration */
	iowrite32(&ns16550_dev.io->lcr, UART_LCR_DLAB);

	/* Configure baudrate */
	divider = baudrate_div_calc(baudrate);
	if (divider < 0) {
		return -1;
	}

	iowrite32(&ns16550_dev.io->dll, divider & 0xFF);
	iowrite32(&ns16550_dev.io->dlh, (divider >> 8) & 0xFF);

	/* 8N1 standard configuration */
	iowrite32(&ns16550_dev.io->lcr, UART_PARITY_DIS | UART_1_STOP | UART_8BITS );

	/* Force RTS and DTR lines */
	iowrite32(&ns16550_dev.io->mcr, UART_RTS | UART_DTR);

	return 0;

}

REGISTER_DRIVER_POSTCORE("serial,ns16550", ns16550_init);
