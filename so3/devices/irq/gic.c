/*
 * Copyright (C) 2014-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

/*
 * Interrupt architecture for the GIC:
 *
 * o There is one Interrupt Distributor, which receives interrupts
 *   from system devices and sends them to the Interrupt Controllers.
 *
 * o There is one CPU Interface per CPU, which sends interrupts sent
 *   by the Distributor, and interrupts generated locally, to the
 *   associated CPU. The base address of the CPU interface is usually
 *   aliased so that the same address points to different chips depending
 *   on the CPU it is accessed from.
 *
 * Note that IRQs 0-31 are special - they are local to each CPU.
 * As such, the enable set/clear, pending set/clear and active bit
 * registers are banked per-cpu for these sources.
 *
 * Some part of code related to GIC virtualization is borrowed from
 * the Jailhouse project.
 */

#if 0
#define DEBUG
#endif

#include <heap.h>
#include <common.h>
#include <memory.h>
#include <spinlock.h>
#include <percpu.h>
#include <errno.h>
#include <smp.h>

#include <device/fdt.h>
#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>

#include <device/arch/gic.h>

#include <device/timer.h>

#include <asm/io.h>
#include <asm/arm_timer.h>

typedef struct {
	/* Distributor */
	struct gicd_regs *gicd;

	/* CPU interface */
	struct gicc_regs *gicc;

#ifdef CONFIG_ARM64VT
	/* Hypervisor related */
	struct gich_regs *gich;

	unsigned int gic_num_lr;
#endif
} gic_t;

static gic_t *gic;

DEFINE_PER_CPU(spinlock_t, intc_lock);

#ifdef CONFIG_ARM64VT

#define MAX_PENDING_IRQS	256

struct pending_irqs {
	/* synchronizes parallel insertions of SGIs into the pending ring */
	spinlock_t lock;

	u16 irqs[MAX_PENDING_IRQS];

	/* contains the calling CPU ID in case of a SGI */
	unsigned int head;

	/* removal from the ring happens lockless, thus tail is volatile */
	volatile unsigned int tail;
};

struct pending_irqs pending_irqs;

static u32 gic_read_lr(unsigned int n)
{
	return ioread32(&gic->gich->lrbase[n]);
}

static void gic_write_lr(unsigned int n, u32 value)
{
	iowrite32(&gic->gich->lrbase[n], value);
}

#endif /* CONFIG_ARM64VT */

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
		irq_def->irq_class = fdt32_to_cpu(p[0]);
		irq_def->irqnr = fdt32_to_cpu(p[1]);
		irq_def->irq_type = fdt32_to_cpu(p[2]);

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

static void gic_mask(unsigned int irq)
{
	int cpu = smp_processor_id();

	spin_lock(&per_cpu(intc_lock, cpu));

	/* Disable/mask IRQ using the clear-enable register */
	iowrite32(&gic->gicd->icenabler[irq/32], 1 << (irq % 32));
	
	spin_unlock(&per_cpu(intc_lock, cpu));
}

static void gic_unmask(unsigned int irq)
{
	int cpu = smp_processor_id();

	spin_lock(&per_cpu(intc_lock, cpu));
	
	/* Enable/unmask IRQ using the set-enable register */
	iowrite32(&gic->gicd->isenabler[irq/32], 1 << (irq % 32));

	spin_unlock(&per_cpu(intc_lock, cpu));

}

void gic_set_prio(unsigned int irq, unsigned char prio)
{
	int cpu = smp_processor_id();
	u32 primask = 0xff << (irq % 4) * 8;
	u32 prival = prio << (irq % 4) * 8;
	u32 prioff = (irq / 4);
	u32 val;

	spin_lock(&per_cpu(intc_lock, cpu));

	val = ioread32(&gic->gicd->ipriorityr[prioff]);
	val &= ~primask;
	val |= prival;
	iowrite32(&gic->gicd->ipriorityr[prioff], val);

	spin_unlock(&per_cpu(intc_lock, cpu));
}

int irq_set_affinity(unsigned int irq, int cpu)
{
	volatile void *reg = &gic->gicd->itargetsr[(irq & ~3)/4];
	unsigned int shift = (irq % 4) * 8;
	u32 val;
	struct irqdesc *desc;
	int __cpu = smp_processor_id();

	spin_lock(&per_cpu(intc_lock, __cpu));
	desc = irq_to_desc(irq);
	if (desc == NULL) {
		spin_unlock(&per_cpu(intc_lock, __cpu));
		BUG();
	}

	val = ioread32(reg) & ~(0xff << shift);
	val |= 1 << (cpu + shift);
	iowrite32(reg, val);
	spin_unlock(&per_cpu(intc_lock, __cpu));

	return 0;
}

void gicc_init(void)
{
	unsigned int cpu = smp_processor_id();
	u32 bypass = 0;
	int i;

	spin_lock_init(&per_cpu(intc_lock, cpu));

	/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	 */
	iowrite32(&gic->gicd->icenabler, GICD_INT_EN_CLR_PPI);
	iowrite32(&gic->gicd->isenabler, GICD_INT_EN_SET_SGI);

	/* Priority for all SGI and PPI interrupts is the highest (value 0) */
	for (i = 0; i < 32; i += 4)
		iowrite32(&gic->gicd->ipriorityr[i/4], 0);

	/* Allow all priorities */
	iowrite32(&gic->gicc->pmr, GICC_INT_PRI_THRESHOLD);

	/*
	 * Preserve bypass disable bits to be written back later
	 */
	bypass = ioread32(&gic->gicc->ctlr);
	bypass &= GICC_DIS_BYPASS_MASK;

	iowrite32(&gic->gicc->ctlr, bypass | GICC_ENABLE | GIC_CPU_EOI);
}

#ifdef CONFIG_ARM64VT

static void gic_enable_maint_irq(bool enable)
{
	u32 hcr;

	hcr = ioread32(&gic->gich->hcr);
	if (enable)
		hcr |= GICH_HCR_UIE;
	else
		hcr &= ~GICH_HCR_UIE;

	iowrite32(&gic->gich->hcr, hcr);
}

static int gic_inject_irq(u16 irq_id)
{
	unsigned int n;
	int first_free = -1;
	u32 lr;
	unsigned long elsr[2];

	elsr[0] = ioread32(&gic->gich->elsr0);
	elsr[1] = ioread32(&gic->gich->elsr1);

	for (n = 0; n < gic->gic_num_lr; n++) {
		if (test_bit(n, elsr)) {
			/* Entry is available */
			if (first_free == -1)
				first_free = n;
			continue;
		}

		/* Check that there is no overlapping */
		lr = gic_read_lr(n);
		if ((lr & GICH_LR_VIRT_ID_MASK) == irq_id)
			return -EEXIST;
	}

	if (first_free == -1)
		return -EBUSY;

	/* Inject group 0 interrupt (seen as IRQ by the guest) */
	lr = irq_id;
	lr |= GICH_LR_PENDING_BIT;

	lr |= GICH_LR_HW_BIT;
	lr |= (u32)irq_id << GICH_LR_PHYS_ID_SHIFT;

	gic_write_lr(first_free, lr);

	return 0;
}

void gic_inject_pending(void)
{
	u16 irq_id;

	while (pending_irqs.head != pending_irqs.tail) {
		irq_id = pending_irqs.irqs[pending_irqs.head];

		if (gic_inject_irq(irq_id) == -EBUSY) {
			/*
			 * The list registers are full, trigger maintenance
			 * interrupt and leave.
			 */
			gic_enable_maint_irq(true);
			return;
		}

		/*
		 * Ensure that the entry was read before updating the head
		 * index.
		 */
		dmb(ish);

		pending_irqs.head = (pending_irqs.head + 1) % MAX_PENDING_IRQS;
	}

	/*
	 * The software interrupt queue is empty - turn off the maintenance
	 * interrupt.
	 */
	gic_enable_maint_irq(false);
}

void gic_set_pending(u16 irq_id)
{
	unsigned int new_tail;

	if (gic_inject_irq(irq_id) != -EBUSY)
		return;

	spin_lock(&pending_irqs.lock);

	new_tail = (pending_irqs.tail + 1) % MAX_PENDING_IRQS;

	/* Queue space available? */
	if (new_tail != pending_irqs.head) {
		pending_irqs.irqs[pending_irqs.tail] = irq_id;

		/*
		 * Make the entry content is visible before updating the tail
		 * index.
		 */
		dmb(ish);

		pending_irqs.tail = new_tail;
	}

	/*
	 * The unlock has memory barrier semantic on ARM v7 and v8. Therefore
	 * the change to tail will be visible when sending SGI_INJECT later on.
	 */
	spin_unlock(&pending_irqs.lock);

	/*
	 * The list registers are full, trigger maintenance interrupt if we are
	 * on the target CPU. In the other case, send SGI_INJECT to the target
	 * CPU.
	 */

	gic_enable_maint_irq(true);
}

#endif /* CONFIG_ARM64VT */

static void gic_eoi_irq(u32 irq_id, bool deactivate)
{
	/*
	 * The GIC doesn't seem to care about the CPUID value written to EOIR,
	 * which is rather convenient...
	 */
	iowrite32(&gic->gicc->eoir, irq_id);
	if (deactivate)
		iowrite32(&gic->gicc->dir, irq_id);
}

static void gic_enable(unsigned int irq) {
	gic_unmask(irq);
}

static void gic_disable(unsigned int irq) {
	gic_mask(irq);
}

/*
 * The interrupt numbering scheme is defined in the
 * interrupt controller spec.  To wit:
 *
 * Interrupts 0-15 are IPI
 * 16-28 are reserved
 * 29-31 are local.  We allow 30 to be used for the watchdog.
 * 32-1020 are global
 * 1021-1022 are reserved
 * 1023 is "spurious" (no interrupt)
 *
 * For now, we ignore all local interrupts so only return an interrupt if it's
 * between 30 and 1020.  The test_for_ipi routine below will pick up on IPIs.
 *
 * A simple read from the controller will tell us the number of the highest
 * priority enabled interrupt.  We then just need to check whether it is in the
 * valid range for an IRQ (30-1020 inclusive).
 *
 */

static void gic_handle(cpu_regs_t *cpu_regs) {
	int irq_nr;
	int irqstat;

#ifdef CONFIG_ARM64VT
	arm_timer_t *arm_timer = (arm_timer_t *) dev_get_drvdata(periodic_timer.dev);
#endif

	do {
		irqstat = ioread32(&gic->gicc->iar);
		irq_nr = irqstat & GICC_IAR_INT_ID_MASK;

		if (irq_nr > 1021)
			break;

		if (irq_nr < 16)
			/* At this leve, IPI are used to trigger a guest because
			 * an event has been raised. So, the domain will check for
			 * this event along the return path.
			 */

			gic_eoi_irq(irq_nr, false);
		else {

#ifdef CONFIG_ARM64VT
			if (irq_nr == arm_timer->irq_def.irqnr) {
				irq_process(irq_nr);
				gic_eoi_irq(irq_nr, false);
			} else {

				if (irq_nr == IRQ_ARCH_ARM_MAINT) {
					gic_inject_pending();
					gic_eoi_irq(irq_nr, true);
					continue;
				} else {
					gic_set_pending(irq_nr);
					gic_eoi_irq(irq_nr, false);
				}
			}
#else
					irq_process(irq_nr);

					gic_eoi_irq(irq_nr, false);
#endif
		}

	} while (true);
}

#ifdef CONFIG_ARM64VT

static void gic_clear_pending_irqs(void)
{
	unsigned int n;

	/* Clear list registers. */
	for (n = 0; n < gic->gic_num_lr; n++)
		gic_write_lr(n, 0);

	/* Clear active priority bits. */
	iowrite32(&gic->gich->apr, 0);
}

#endif /* CONFIG_ARM64VT */

void smp_cross_call(long cpu_mask, unsigned int irq)
{
	unsigned long flags;
	int cpu = smp_processor_id();

	flags = spin_lock_irqsave(&per_cpu(intc_lock, cpu));

	/*
	 * Ensure that stores to Normal memory are visible to the
	 * other CPUs before they observe us issuing the IPI.
	 */
	smp_mb();

	/* This always happens on GIC0 */
	iowrite32(&gic->gicd->sgir, (cpu_mask << 16) | irq);

	spin_unlock_irqrestore(&per_cpu(intc_lock, cpu), flags);
}

void gic_set_type(unsigned int irq, unsigned int type)
{
	u32 confmask = 0x2 << ((irq % 16) * 2);
	u32 val, oldval;

	/*
	 * Read current configuration register, and insert the config
	 * for "irq", depending on "type".
	 */

	val = oldval = ioread32(&gic->gicd->icfgr[irq/16]);

	if (type & IRQ_TYPE_LEVEL_MASK)
		val &= ~confmask;
	else if (type & IRQ_TYPE_EDGE_BOTH)
		val |= confmask;

	/* If the current configuration is the same, then we are done */
	if (val == oldval)
		return ;

	iowrite32(&gic->gicd->icfgr[irq/16], val);

}

static int gic_init(dev_t *dev, int fdt_offset) {
	const struct fdt_property *prop;
	int prop_len;
	u32 gicc_ctlr, gicc_pmr;
	u32 gicd_isacter;
	unsigned int n;

#ifdef CONFIG_ARM64VT
	u32 vtr, vmcr;
#endif

	gic = (gic_t *) malloc(sizeof(gic_t));
	BUG_ON(!gic);

	DBG("%s\n", __FUNCTION__);

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);

#if defined(CONFIG_AVZ) && defined(CONFIG_ARM64VT)
	BUG_ON(prop_len != 6 * sizeof(unsigned long));
#else
	BUG_ON(prop_len != 4 * sizeof(unsigned long));
#endif

	/* Mapping the two mem area of GIC (distributor & CPU interface) */
#ifdef CONFIG_ARCH_ARM32
	gic->gicd = (struct gicd_regs *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]), fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
	gic->gicc = (struct gicc_regs *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[2]), fdt32_to_cpu(((const fdt32_t *) prop->data)[3]));
#else
	gic->gicd = (struct gicd_regs *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]), fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));
	gic->gicc = (struct gicc_regs *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[2]), fdt64_to_cpu(((const fdt64_t *) prop->data)[3]));
#endif

#ifdef CONFIG_ARM64VT
	gic->gich = (void *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[4]), fdt64_to_cpu(((const fdt64_t *) prop->data)[5]));

	spin_lock_init(&pending_irqs.lock);

	/* Disable PPIs, except for the maintenance interrupt. */
	iowrite32(&gic->gicd->isenabler, 0xffff0000 & ~(1 << IRQ_ARCH_ARM_MAINT));

	/* Ensure all IPIs and the maintenance PPI are enabled */
	iowrite32(&gic->gicd->isenabler, 0x0000ffff & ~(1 << IRQ_ARCH_ARM_MAINT));

	/* Deactivate all active PPIs */
	iowrite32(&gic->gicd->icenabler, 0xffff0000);

	iowrite32(&gic->gich->vmcr, 0);

	pending_irqs.head = 0;
	pending_irqs.tail = 0;

	gic_clear_pending_irqs();

#endif

	gicc_ctlr = ioread32(&gic->gicc->ctlr);
	gicc_pmr = ioread32(&gic->gicc->pmr);

#ifdef CONFIG_ARM64VT

	vtr = ioread32(&gic->gich->vtr);
	gic->gic_num_lr = (vtr & 0x3f) + 1;

	/* VMCR only contains 5 bits of priority */
	vmcr = (gicc_pmr >> GICV_PMR_SHIFT) << GICH_VMCR_PMR_SHIFT;
	/*
	 * All virtual interrupts are group 0 in this driver since the GICV
	 * layout seen by the guest corresponds to GICC without security
	 * extensions:
	 * - A read from GICV_IAR doesn't acknowledge group 1 interrupts
	 *   (GICV_AIAR does it, but the guest never attempts to accesses it)
	 * - A write to GICV_CTLR.GRP0EN corresponds to the GICC_CTLR.GRP1EN bit
	 *   Since the guest's driver thinks that it is accessing a GIC with
	 *   security extensions, a write to GPR1EN will enable group 0
	 *   interrupts.
	 * - Group 0 interrupts are presented as virtual IRQs (FIQEn = 0)
	 */

	if (gicc_ctlr & GICC_CTLR_GRPEN1)
		vmcr |= GICH_VMCR_EN0;
	if (gicc_ctlr & GICC_CTLR_EOImode)
		vmcr |= GICH_VMCR_EOImode;

	iowrite32(&gic->gich->vmcr, vmcr);

	iowrite32(&gic->gich->hcr, GICH_HCR_EN);

	/*
	 * Clear pending virtual IRQs in case anything is left from previous
	 * use. Physically pending IRQs will be forwarded to Linux once we
	 * enable interrupts for the hypervisor, except for SGIs, see below.
	 */
	gic_clear_pending_irqs();

#endif

	/* Deactivate all active SGIs */
	gicd_isacter = ioread32(&gic->gicd->isactiver);
	iowrite32(&gic->gicd->isactiver, gicd_isacter & 0xffff);

#ifdef CONFIG_ARM64VT
	/*
	 * Forward any pending physical SGIs to the virtual queue.
	 * We will convert them into self-inject SGIs, ignoring the original
	 * source. But Linux doesn't care about that anyway.
	 */
	for (n = 0; n < 16; n++) {
		if (ioread8(((u8 *) &gic->gicd->cpendsgirn) + n)) {
			iowrite8(((u8 *) &gic->gicd->cpendsgirn) + n, 0xff);
			gic_set_pending(n);
		}
	}

#else /* CONFIG_ARM64VT */

	/* Initialize distributor and CPU interface of GIC.
	 * See Linux implementation as reference: http://lxr.free-electrons.com/source/arch/arm/common/gic.c?v=3.2
	 */

	/* Distributor interface initialization */

	/* Disable distributor */
	iowrite32(&gic->gicd->ctlr, ioread32(((void *) &gic->gicd->ctlr) + INTC_CPU_CTRL_REG0) & ~INTC_DISABLE);

	/* All interrupts level triggered, active high by default */
	for (n = 32; n < NR_IRQS; n++)
		gic_set_type(n, IRQ_TYPE_LEVEL_HIGH);

	/* Target CPU for all IRQs is CPU0 */
	for (n = 32; n < NR_IRQS; n += 4) {
		iowrite32(&gic->gicd->itargetsr[n/4], 0x01010101);
	}

	/* Priority for all interrupts is the highest (value 0) */
	for (n = 32; n < NR_IRQS; n += 4) {
		iowrite32(&gic->gicd->ipriorityr[n/4], 0);
	}

	/* Disable all interrupts  */
	for (n = 32; n < NR_IRQS; n += 32) {
		iowrite32(&gic->gicd->icenabler[n/32], 0xffffffff);
	}

	/* Enable distributor */
	iowrite32(&gic->gicd->ctlr, GICD_ENABLE);

	/* CPU interface initialization */

	/* Handle SGI (0-15) and PPI interrupts (16-31) separately */
	/* Disable all PPI and SGI interrupts */
	iowrite32(&gic->gicd->icenabler[0], 0xffffffff);

	/* Priority for all SGI and PPI interrupts is the highest (value 0) */
	for (n = 0; n < 32; n += 4) {
		iowrite32(&gic->gicd->ipriorityr[n/4], 0);
	}

	/* Allow all priorities */
	iowrite32(&gic->gicc->pmr, 0xff);

	/* Enable CPU interface */
	iowrite32(&gic->gicc->ctlr, GICC_ENABLE);

#endif /* !CONFIG_ARM64VT */

	irq_ops.enable = gic_enable;
	irq_ops.disable = gic_disable;
	irq_ops.mask = gic_mask;
	irq_ops.unmask = gic_unmask;
	irq_ops.handle = gic_handle;

	return 0;
}

REGISTER_DRIVER_CORE("intc,gic", gic_init);

