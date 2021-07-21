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

#include <device/arch/plic.h>

#include <asm/io.h>


#define PLIC_PRIO_OFFSET		0x4 		/* IRQ 0 does not exist, starts at 1 */
#define PLIC_PENDING_OFFSET		0x1000
#define PLIC_ENABLE_OFFSET		0x2000
#define PLIC_THRESH_OFFSET		0x200000
#define PLIC_CLAIM_OFFSET		0x200004

static volatile u32 *base_addr;

static void plic_mask(unsigned int irq) {



#if 0
	/* Disable/mask IRQ using the clear-enable register */
	iowrite32(&regs->gicd_icenabler[irq/32], 1 << (irq % 32));

	newvalue={currentvalue& (¬(0×1<<(intid% 32)))}
#endif
}

static void plic_unmask(unsigned int irq) {

	u32 *enable_base 	= (void *) base_addr + PLIC_ENABLE_OFFSET;

	u32 tmp = enable_base[irq/32];

	tmp |= ( 0x1 << irq % 32);

	enable_base[irq/32] = tmp;
}

static void plic_enable(unsigned int irq) {
	plic_unmask(irq);
}

static void plic_disable(unsigned int irq) {
	plic_mask(irq);
}

static void plic_handle(cpu_regs_t *cpu_regs) {
#if 0
	int irq_nr;
	int irqstat;

	do {
		irqstat = ioread32(&regs->gicc_iar);
		irq_nr = irqstat & GICC_IAR_INT_ID_MASK;

		if ((irq_nr < 15) || (irq_nr > 1021))
			break;

		irq_process(irq_nr);

		iowrite32(&regs->gicc_eoir, irq_nr);

	} while (true);
#endif
}

static int plic_init(dev_t *dev) {

	int i;
	u32 *enable_base;
	u32 *prio_base;
	u32 *thresh_base;

	base_addr = (volatile u32*) dev->base;

	enable_base 	= (void *) base_addr + PLIC_ENABLE_OFFSET;
	prio_base	 	= (void *) base_addr + PLIC_PRIO_OFFSET;
	thresh_base 	= (void *) base_addr + PLIC_THRESH_OFFSET;

	/* Ensure that interrupts are disabled. There are 1023 irqs (irq 0 does not exist but is mapped)
	 * and there are 32 bits per register to set to 0. */
	for (i = 0; i < 1024/32; i++) {
			*enable_base = 0x0;
			enable_base++;
		}

	/* Set priority of each interrupt to 0 (prio is stored on 32 bits) */
	for (i = 0; i < 1024; i++) {
		*prio_base = 0x0;
		prio_base++;
	}

	/* Set threshold priority to 0 */
	*thresh_base = 0x0;

	irq_ops.irq_enable = plic_enable;
	irq_ops.irq_disable = plic_disable;
	irq_ops.irq_mask = plic_mask;
	irq_ops.irq_unmask = plic_unmask;
	irq_ops.irq_handle = plic_handle;

	return 0;
}

REGISTER_DRIVER_CORE("intc,plic", plic_init);
