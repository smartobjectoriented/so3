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

#include <device/arch/bcm283x_mu.h>
#include <mach/io.h>

#include <asm/io.h>                 /* ioread/iowrite macros */

void *__uart_vaddr = (void *) CONFIG_UART_LL_PADDR;

typedef struct {
	addr_t base;
	bcm283x_mu_t *dev;
} bcm283x_mu_dev_t;

static bcm283x_mu_dev_t bcm283x_mu_dev =
{
  .base = CONFIG_UART_LL_PADDR,
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

void printch(char c) {
	bcm283x_mu_put_byte(c);
	
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

static int bcm283x_mu_init(dev_t *dev, int fdt_offset)
{
	const struct fdt_property *prop;
	int prop_len;

	/* Pins multiplexing skipped here for simplicity (done by bootloader) */
	/* Clocks init skipped here for simplicity (done by bootloader) */

	/* Initialize UART controller */
	memset(&bcm283x_mu_dev, 0, sizeof(bcm283x_mu_dev_t));

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);

	BUG_ON(prop_len != 2 * sizeof(unsigned long));

#ifdef CONFIG_ARCH_ARM32
	bcm283x_mu_dev.base = io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]), fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
#else
	bcm283x_mu_dev.base = io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]), fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));
#endif

	serial_ops.put_byte = bcm283x_mu_put_byte;
	serial_ops.get_byte = bcm283x_mu_get_byte;

	return 0;

}

REGISTER_DRIVER_POSTCORE("serial,bcm283x-mu", bcm283x_mu_init);
