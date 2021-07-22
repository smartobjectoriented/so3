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

#if 0
#define DEBUG
#endif

#include <common.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>

#include <asm/io.h>

#define PLIC_PRIO_OFFSET		0x0
#define PLIC_PENDING_OFFSET		0x1000
#define PLIC_ENABLE_OFFSET		0x2000
#define PLIC_THRESH_OFFSET		0x200000
#define PLIC_CLAIM_OFFSET		0x200004

static volatile u32 *base_addr;

static void plic_mask(unsigned int irq) {

	irq -= 32;

	u32 *enable_base 	= (void *) base_addr + PLIC_ENABLE_OFFSET;
	u32 *prio_base	 	= (void *) base_addr + PLIC_PRIO_OFFSET;

	/* Enable irq */
	u32 tmp = ioread32(enable_base + irq/32);
	tmp &= ~( 0x1 << (irq % 32));
	iowrite32(enable_base + irq/32, tmp);

	/* Reset prio to 0 again */
	iowrite32(prio_base + irq, 0x0);
}

static void plic_unmask(unsigned int irq) {

	irq -= 32;

	u32 *enable_base 	= (void *) base_addr + PLIC_ENABLE_OFFSET;
	u32 *prio_base	 	= (void *) base_addr + PLIC_PRIO_OFFSET;

	/* Clear enable bit */
	u32 tmp = ioread32(enable_base + irq/32);
	tmp |= ( 0x1 << (irq % 32));
	iowrite32(enable_base + irq/32, tmp);

	/* Set prio to 1 because 0 will be ignored */
	iowrite32(prio_base + irq, 0x1);
}

static void plic_enable(unsigned int irq) {
	plic_unmask(irq);
}

static void plic_disable(unsigned int irq) {
	plic_mask(irq);
}

static void plic_handle(cpu_regs_t *cpu_regs) {

	int irq_nr;
	u32 *claim_base = (void *)base_addr + PLIC_CLAIM_OFFSET;

	do {
		/* Get number of highest prio irq */
		irq_nr = ioread32(claim_base);

		/* Read returns 0 if no irq is pending anymore */
		if (!irq_nr)
			break;

		/* Process irq */
		irq_process(irq_nr + 32);

		/* Write irq back to claim reg to ack the interrupt */
		iowrite32(claim_base, irq_nr);

	} while (true);
}

static int plic_init(dev_t *dev) {

	int i;
	u32 *enable_base;
	u32 *prio_base;
	u32 *thresh_base;

	base_addr = (volatile u32*) dev->base;

	enable_base 	= (void *)base_addr + PLIC_ENABLE_OFFSET;
	prio_base	 	= (void *)base_addr + PLIC_PRIO_OFFSET;
	thresh_base 	= (void *)base_addr + PLIC_THRESH_OFFSET;

	/* Ensure that interrupts are disabled. There are 1023 irqs (irq 0 does not exist but is mapped)
	 * and there are 32 bits per register to set to 0. */
	for (i = 0; i < 1024/32; i++) {
		iowrite32(enable_base, 0x0);
		enable_base++;
	}

	/* Set priority of each interrupt to 0 (prio is stored on 32 bits). IRQ 0 doesn't
	 * exist. Which means we have to start at offset 0x4 for IRQ number 1 */
	prio_base++;
	for (i = 1; i < 1024; i++) {
		iowrite32(prio_base, 0x0);
		prio_base++;
	}

	/* Set threshold priority to 0 */
	iowrite32(thresh_base, 0x0);

	irq_ops.irq_enable = plic_enable;
	irq_ops.irq_disable = plic_disable;
	irq_ops.irq_mask = plic_mask;
	irq_ops.irq_unmask = plic_unmask;
	irq_ops.irq_handle = plic_handle;



	/* Enable EXT irqs in status register */
	csr_set(CSR_IE, IE_EIE);

	return 0;
}

REGISTER_DRIVER_CORE("intc,plic", plic_init);
