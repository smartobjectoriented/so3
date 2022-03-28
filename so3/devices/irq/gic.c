/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2014 Romain Bornet <romain.bornet@heig-vd.ch>
 * Copyright (C) 2016-2017 Alexandre Malki <alexandre.malki@heig-vd.ch>
 *
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

#include <heap.h>
#include <common.h>
#include <memory.h>

#include <device/fdt.h>
#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>

#include <device/arch/gic.h>

#include <asm/io.h>

typedef struct {
	/* Distributor */
	addr_t gicd_base;
	uint32_t gicd_size;

	/* CPU interface */
	addr_t gicc_base;
	uint32_t gicc_size;

	struct gicd_regs *gicd;
	struct gicc_regs *gicc;

} gic_t;

static gic_t *gic;

/**
 * Retrieve the information related to an interrupt entry from the DT.
 *
 * @param fdt_offset
 * @param irq_def
 */
void fdt_interrupt_node(int fdt_offset, irq_def_t *irq_def) {
	int prop_len;
	const struct fdt_property *prop;
	const fdt32_t *p;

	/* Interrupts - as described in the bindings - have 3 specific cells */
	prop = fdt_get_property(__fdt_addr, fdt_offset, "interrupts", &prop_len);
	BUG_ON(!prop);

	p = (const fdt32_t *) prop->data;

	if (prop_len == 3 * sizeof(uint32_t)) {

		/* Retrieve the 3-cell values */
		irq_def->irqnr = fdt32_to_cpu(p[0]);
		irq_def->irq_class = fdt32_to_cpu(p[1]);
		irq_def->irq_type = fdt32_to_cpu(p[3]);

		/* Not all combinations are currently handled. */

		if (irq_def->irq_class != GIC_IRQ_TYPE_SGI)
			irq_def->irqnr += 16; /* Possibly for a Private Peripheral Interrupt (PPI) */

		if (irq_def->irq_class == GIC_IRQ_TYPE_SPI) /* It is a Shared Peripheral Interrupt (SPI) */
			irq_def->irqnr += 16;

	} else {
		/* Unsupported size of interrupts property */
		lprintk("%s: unsupported size of interrupts property\n");
		BUG();
	}

}

static void gic_mask(unsigned int irq) {

	/* Disable/mask IRQ using the clear-enable register */
	iowrite32(&gic->gicd->gicd_icenabler[irq/32], 1 << (irq % 32));
}

static void gic_unmask(unsigned int irq) {

	/* Enable/unmask IRQ using the set-enable register */
	iowrite32(&gic->gicd->gicd_isenabler[irq/32], 1 << (irq % 32));
}

static void gic_enable(unsigned int irq) {
	gic_unmask(irq);
}

static void gic_disable(unsigned int irq) {
	gic_mask(irq);
}

static void gic_handle(cpu_regs_t *cpu_regs) {
	int irq_nr;
	int irqstat;

	do {
		irqstat = ioread32(&gic->gicc->gicc_iar);
		irq_nr = irqstat & GICC_IAR_INT_ID_MASK;

		if ((irq_nr < 15) || (irq_nr > 1021))
			break;

		irq_process(irq_nr);

		iowrite32(&gic->gicc->gicc_eoir, irq_nr);

	} while (true);
}

void gic_set_type(unsigned int irq, unsigned int type)
{
	u32 confmask = 0x2 << ((irq % 16) * 2);
	u32 val, oldval;

	/*
	 * Read current configuration register, and insert the config
	 * for "irq", depending on "type".
	 */

	val = oldval = ioread32(&gic->gicd->gicd_icfgr[irq/16]);

	if (type & IRQ_TYPE_LEVEL_MASK)
		val &= ~confmask;
	else if (type & IRQ_TYPE_EDGE_BOTH)
		val |= confmask;

	/* If the current configuration is the same, then we are done */
	if (val == oldval)
		return ;

	iowrite32(&gic->gicd->gicd_icfgr[irq/16], val);

}

static int gic_init(dev_t *dev, int fdt_offset) {
	int i;
	const struct fdt_property *prop;
	int prop_len;

#ifdef CONFIG_ARCH_ARM32
	const fdt32_t *p;
#else
	const fdt64_t *p;
#endif

	gic = (gic_t *) malloc(sizeof(gic_t));
	BUG_ON(!gic);

	DBG("%s\n", __FUNCTION__);

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);
	BUG_ON(prop_len != 4 * sizeof(unsigned long));


#ifdef CONFIG_ARCH_ARM32
	p = (const fdt32_t *) prop->data;

	gic->gicd_base = fdt32_to_cpu(p[0]);
	gic->gicd_size = fdt32_to_cpu(p[1]);
#else
	p = (const fdt64_t *) prop->data;

	gic->gicd_base = fdt64_to_cpu(p[0]);
	gic->gicd_size = fdt64_to_cpu(p[1]);
#endif

	/* Mapping the two mem area of GIC (distributor & CPU interface) */
	gic->gicd_base = io_map(gic->gicd_base, gic->gicd_size);
	gic->gicc_base = io_map(gic->gicc_base, gic->gicc_size);

	gic->gicd = (struct gicd_regs *) gic->gicd_base;
	gic->gicc = (struct gicc_regs *) gic->gicc_base;

	/* Initialize distributor and CPU interface of GIC.
	 * See Linux implementation as reference: http://lxr.free-electrons.com/source/arch/arm/common/gic.c?v=3.2
	 */

	/* Distributor interface initialization */

	/* Disable distributor */
	iowrite32(&gic->gicd->gicd_ctlr, ioread32(((void *) &gic->gicd->gicd_ctlr) + INTC_CPU_CTRL_REG0) & ~INTC_DISABLE);

	/* All interrupts level triggered, active high by default */
	for (i = 32; i < NR_IRQS; i++)
		gic_set_type(i, IRQ_TYPE_LEVEL_HIGH);

	/* Target CPU for all IRQs is CPU0 */
	for (i = 32; i < NR_IRQS; i += 4) {
		iowrite32(&gic->gicd->gicd_itargetsr[i/4], 0x01010101);
	}

	/* Priority for all interrupts is the highest (value 0) */
	for (i = 32; i < NR_IRQS; i += 4) {
		iowrite32(&gic->gicd->gicd_ipriorityr[i/4], 0);
	}

	/* Disable all interrupts  */
	for (i = 32; i < NR_IRQS; i += 32) {
		iowrite32(&gic->gicd->gicd_icenabler[i/32], 0xffffffff);
	}

	/* Enable distributor */
	iowrite32(&gic->gicd->gicd_ctlr, GICD_ENABLE);

	/* CPU interface initialization */

	/* Handle SGI (0-15) and PPI interrupts (16-31) separately */
	/* Disable all PPI and SGI interrupts */
	iowrite32(&gic->gicd->gicd_icenabler[0], 0xffffffff);

	/* Priority for all SGI and PPI interrupts is the highest (value 0) */
	for (i = 0; i < 32; i += 4) {
		iowrite32(&gic->gicd->gicd_ipriorityr[i/4], 0);
	}

	/* Allow all priorities */
	iowrite32(&gic->gicc->gicc_pmr, 0xff);

	/* Enable CPU interface */
	iowrite32(&gic->gicc->gicc_ctlr, GICC_ENABLE);

	irq_ops.irq_enable = gic_enable;
	irq_ops.irq_disable = gic_disable;
	irq_ops.irq_mask = gic_mask;
	irq_ops.irq_unmask = gic_unmask;
	irq_ops.irq_handle = gic_handle;

	return 0;
}

REGISTER_DRIVER_CORE("intc,gic", gic_init);

