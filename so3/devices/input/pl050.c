/*
 * Copyright (C) 2020 Nikolaos Garanis <nikolaos.garanis@heig-vd.ch>
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
 * This is a driver for the PL050 controller. There are two such controllers on
 * the Versatile Express board, one for the mouse and one for the keyboard. The
 * differences are the base addresses and the interrupt numbers.
 *
 * kmi0.c is the driver for the keyboard and kmi1.c for the mouse.
 *
 * Documentation:
 *   http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0143c/index.html
 *   https://wiki.osdev.org/PL050_PS/2_Controller
 */

#include <printk.h>
#include <vfs.h>
#include <asm/io.h>
#include <device/input/pl050.h>


/* Write a byte to the device. */
void pl050_write(void *base, uint8_t data)
{
	/* Check if we can actually write in the transmit registry. */
	uint8_t status = ioread8(base + KMI_STAT);
	if (0 == (status & KMISTAT_TXEMPTY)) {
		printk("%s: write timeout.\n", __func__);
		return;
	}

	iowrite8(base + KMI_DATA, data);
}

/*
 * Initialisation of the PL050 Keyboard/Mouse Interface.
 * Linux driver: input/serio/ambakmi.c
 */
void pl050_init(void *base, irq_def_t *irq_def, irq_return_t (*isr)(int, void *))
{

	/* Set the clock divisor (arbitrary value). */
	iowrite8(base + KMI_CLKDIV, 2);

	/* Enable interface and receiver interrupt. */
	iowrite8(base + KMI_CR, KMICR_EN | KMICR_RXINTREN | KMICR_TXINTREN);

	/* Bind the ISR to the interrupt controller. */
	irq_bind(irq_def->irqnr, isr, NULL, NULL);

}
