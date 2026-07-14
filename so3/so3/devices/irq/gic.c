/*
 * Copyright (C) 2014-2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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

#include <common.h>
#include <heap.h>
#include <memory.h>
#include <percpu.h>
#include <smp.h>
#include <spinlock.h>
#include <log.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/fdt.h>
#include <device/irq.h>

#include <device/arch/gic.h>

#include <device/timer.h>

#include <asm/arm_timer.h>
#include <asm/io.h>

#ifdef CONFIG_ARM64
#include <asm/virt.h>
#endif /* CONFIG_ARM64 */

#ifdef CONFIG_AVZ
#include <avz/sched.h>
#endif

gic_t *gic;

DEFINE_PER_CPU(spinlock_t, intc_lock);

DECLARE_PER_CPU(unsigned long, gic_iar_count);
DECLARE_PER_CPU(unsigned long, gic_inj_27_count);
DECLARE_PER_CPU(unsigned long, gic_inj_total_count);
DECLARE_PER_CPU(unsigned long, gic_busy_count);
DECLARE_PER_CPU(unsigned long, gic_eexist_count);
DECLARE_PER_CPU(unsigned long, gic_sgi0_recv);
DECLARE_PER_CPU(unsigned long, gic_sgi0_eexist);

#ifdef CONFIG_AVZ

#define MAX_PENDING_IRQS 256

struct pending_irqs {
	/* synchronizes parallel insertions of SGIs into the pending ring */
	spinlock_t lock;

	u16 irqs[MAX_PENDING_IRQS];

	/* contains the calling CPU ID in case of a SGI */
	unsigned int head;

	/* removal from the ring happens lockless, thus tail is volatile */
	volatile unsigned int tail;
};

DEFINE_PER_CPU(struct pending_irqs, pending_irqs);

#ifdef CONFIG_GIC_V3
/* GICv3: list registers are ICH_LR*_EL2 system registers (64-bit) */
static u64 gic_read_lr(unsigned int n)
{
	switch (n) {
	case 0:
		return read_sysreg_s(SYS_ICH_LR0_EL2);
	case 1:
		return read_sysreg_s(SYS_ICH_LR1_EL2);
	case 2:
		return read_sysreg_s(SYS_ICH_LR2_EL2);
	case 3:
		return read_sysreg_s(SYS_ICH_LR3_EL2);
	case 4:
		return read_sysreg_s(SYS_ICH_LR4_EL2);
	case 5:
		return read_sysreg_s(SYS_ICH_LR5_EL2);
	case 6:
		return read_sysreg_s(SYS_ICH_LR6_EL2);
	case 7:
		return read_sysreg_s(SYS_ICH_LR7_EL2);
	case 8:
		return read_sysreg_s(SYS_ICH_LR8_EL2);
	case 9:
		return read_sysreg_s(SYS_ICH_LR9_EL2);
	case 10:
		return read_sysreg_s(SYS_ICH_LR10_EL2);
	case 11:
		return read_sysreg_s(SYS_ICH_LR11_EL2);
	case 12:
		return read_sysreg_s(SYS_ICH_LR12_EL2);
	case 13:
		return read_sysreg_s(SYS_ICH_LR13_EL2);
	case 14:
		return read_sysreg_s(SYS_ICH_LR14_EL2);
	case 15:
		return read_sysreg_s(SYS_ICH_LR15_EL2);
	default:
		return 0;
	}
}

void gic_write_lr(unsigned int n, u64 value)
{
	switch (n) {
	case 0:
		write_sysreg_s(value, SYS_ICH_LR0_EL2);
		break;
	case 1:
		write_sysreg_s(value, SYS_ICH_LR1_EL2);
		break;
	case 2:
		write_sysreg_s(value, SYS_ICH_LR2_EL2);
		break;
	case 3:
		write_sysreg_s(value, SYS_ICH_LR3_EL2);
		break;
	case 4:
		write_sysreg_s(value, SYS_ICH_LR4_EL2);
		break;
	case 5:
		write_sysreg_s(value, SYS_ICH_LR5_EL2);
		break;
	case 6:
		write_sysreg_s(value, SYS_ICH_LR6_EL2);
		break;
	case 7:
		write_sysreg_s(value, SYS_ICH_LR7_EL2);
		break;
	case 8:
		write_sysreg_s(value, SYS_ICH_LR8_EL2);
		break;
	case 9:
		write_sysreg_s(value, SYS_ICH_LR9_EL2);
		break;
	case 10:
		write_sysreg_s(value, SYS_ICH_LR10_EL2);
		break;
	case 11:
		write_sysreg_s(value, SYS_ICH_LR11_EL2);
		break;
	case 12:
		write_sysreg_s(value, SYS_ICH_LR12_EL2);
		break;
	case 13:
		write_sysreg_s(value, SYS_ICH_LR13_EL2);
		break;
	case 14:
		write_sysreg_s(value, SYS_ICH_LR14_EL2);
		break;
	case 15:
		write_sysreg_s(value, SYS_ICH_LR15_EL2);
		break;
	default:
		break;
	}
}
#else /* GICv2: list registers are GICH MMIO */
static u32 gic_read_lr(unsigned int n)
{
	return ioread32(&gic->gich->lr[n]);
}

void gic_write_lr(unsigned int n, u32 value)
{
	iowrite32(&gic->gich->lr[n], value);
}
#endif /* CONFIG_GIC_V3 */

void display_lr(unsigned int n)
{
#ifdef CONFIG_GIC_V3
	u64 lr = gic_read_lr(n);

	printk("LR%u: virq=%lx state=%lx hw=%lx grp=%lx prio=%lx pirq=%lx\n", n, lr & 0xfffff, (lr >> 61) & 0x3,
	       (lr >> 60) & 0x1, (lr >> 59) & 0x1, (lr >> 48) & 0xff, (lr >> 32) & 0xffff);
#else
	u32 lr = gic_read_lr(n);

	printk("LR state: \n");
	printk("  - virq: %x\n", lr & GICH_LR_VIRT_ID_MASK);
	printk("  - prio: %x\n", (lr >> GICH_LR_PRIORITY_SHIFT) & GICH_LR_PRIORITY_MASK);
	printk("  - pending: %x\n", lr & GICH_LR_PENDING_BIT);
	printk("  - active: %x\n", lr & GICH_LR_ACTIVE_BIT);
	printk("  - hw: %x\n", lr & GICH_LR_HW_BIT);
#endif
}

#endif /* CONFIG_AVZ */

/**
 * Retrieve the information related to an interrupt entry from the DT.
 *
 * @param fdt_offset
 * @param irq_def
 */
void fdt_interrupt_node(int fdt_offset, irq_def_t *irq_def)
{
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
		LOG_ERROR("unsupported size of interrupts property\n");
		BUG();
	}
}

static void gic_mask(unsigned int irq)
{
	int cpu = smp_processor_id();

	spin_lock(&per_cpu(intc_lock, cpu));

	/* Disable/mask IRQ using the clear-enable register */
	iowrite32(&gic->gicd->icenabler[irq / 32], 1 << (irq % 32));

	spin_unlock(&per_cpu(intc_lock, cpu));
}

static void gic_unmask(unsigned int irq)
{
	int cpu = smp_processor_id();

	spin_lock(&per_cpu(intc_lock, cpu));

	/* Enable/unmask IRQ using the set-enable register */
	iowrite32(&gic->gicd->isenabler[irq / 32], 1 << (irq % 32));

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
	volatile void *reg = &gic->gicd->itargetsr[(irq & ~3) / 4];
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

#ifdef CONFIG_AVZ

static void gic_enable_maint_irq(bool enable)
{
#ifdef CONFIG_GIC_V3
	u64 hcr = read_sysreg_s(SYS_ICH_HCR_EL2);

	if (enable)
		hcr |= GICH_HCR_UIE;
	else
		hcr &= ~GICH_HCR_UIE;

	write_sysreg_s(hcr, SYS_ICH_HCR_EL2);
#else
	u32 hcr;

	hcr = ioread32(&gic->gich->hcr);
	if (enable)
		hcr |= GICH_HCR_UIE;
	else
		hcr &= ~GICH_HCR_UIE;

	iowrite32(&gic->gich->hcr, hcr);
#endif
}

/* Encode the SGI source CPU in bits[15:13] of the u16 irq_id passed to
 * gic_set_pending() / gic_inject_irq().  INTIDs only need bits[9:0], so the
 * upper bits are free.  Decoded only for SGIs (irq_id < 16); ignored for
 * PPIs/SPIs.  This avoids touching every internal call site of
 * gic_set_pending() while still letting the GICv2 vGIC LR carry the real
 * source CPU instead of smp_processor_id() of the receiving CPU. */
#define GIC_SGI_SRC_CPU_SHIFT 13
#define GIC_SGI_SRC_CPU_MASK (0x7 << GIC_SGI_SRC_CPU_SHIFT)
#define GIC_SGI_PACK(intid, src_cpu) ((u16) ((intid) | (((src_cpu) & 0x7) << GIC_SGI_SRC_CPU_SHIFT)))

static int gic_inject_irq(u16 irq_id_packed)
{
	unsigned int n;
	int first_free = -1;
	u16 irq_id = irq_id_packed & 0x3ff;
	u8 src_cpu = (irq_id_packed >> GIC_SGI_SRC_CPU_SHIFT) & 0x7;

#ifdef CONFIG_GIC_V3
	unsigned long elsr = read_sysreg_s(SYS_ICH_ELRSR_EL2);
	u64 lr64;

	for (n = 0; n < gic->gic_num_lr; n++) {
		if (elsr & (1UL << n)) {
			if (first_free == -1)
				first_free = n;
			continue;
		}

		/* Check for duplicate vINTID (lower 13 bits of LR) */
		if ((gic_read_lr(n) & 0x1fff) == irq_id)
			return -EEXIST;
	}

	if (first_free == -1)
		return -EBUSY;

	if (is_sgi(irq_id)) {
		/* SGIs are software-backed: EL2 already wrote EOIR before injection */
		lr64 = GICH_LR_STATE_PENDING64 | GICH_LR_GRP1_BIT64 |
		       ((u64) GICH_LR_DEFAULT_PRIORITY << GICH_LR_PRIORITY_SHIFT64) | (u64) irq_id;
	} else {
		/* PPIs and SPIs: hardware-backed (HW=1). EL2 does NOT write EOIR.
		 * The physical INTID stays Active, preventing level-triggered re-delivery
		 * storms. The vGIC hardware deactivates the physical INTID automatically
		 * when Linux writes ICV_EOIR1_EL1. pINTID = vINTID (1:1 pass-through). */
		lr64 = GICH_LR_STATE_PENDING64 | GICH_LR_GRP1_BIT64 | GICH_LR_HW_BIT64 |
		       ((u64) GICH_LR_DEFAULT_PRIORITY << GICH_LR_PRIORITY_SHIFT64) |
		       ((u64) irq_id << GICH_LR_PHYS_ID_SHIFT64) | (u64) irq_id;
	}

	u64 hcr = read_sysreg_s(SYS_ICH_HCR_EL2);
	if (!(hcr & GICH_HCR_EN))
		write_sysreg_s(hcr | GICH_HCR_EN, SYS_ICH_HCR_EL2);

	gic_write_lr(first_free, lr64);

#else /* CONFIG_GIC_V2 */
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
		if ((lr & GICH_LR_VIRT_ID_MASK) == irq_id) {
			/* A vSGI of this id is already handled by the guest (LR
			 * ACTIVE): a fresh edge just arrived — possibly from a
			 * different source CPU (SGI source is not part of the id
			 * we match on). Dropping it (-EEXIST) loses an IPI: the
			 * guest has already scanned its call-single queue, so the
			 * new csd never runs and smp_call_function_many() hangs
			 * (RCU stall). Instead re-arm the LR ACTIVE+PENDING so the
			 * guest re-enters the handler after it writes vEOIR and
			 * drains the new work. Only ACTIVE (not plain PENDING)
			 * needs this: a still-PENDING vSGI has not been consumed
			 * yet, so coalescing is correct. */

			if (is_sgi(irq_id) && (lr & GICH_LR_ACTIVE_BIT) && !(lr & GICH_LR_PENDING_BIT)) {
				gic_write_lr(n, lr | GICH_LR_PENDING_BIT);
				return 0;
			}
			this_cpu(gic_eexist_count)++;
			if (irq_id == 0)
				this_cpu(gic_sgi0_eexist)++;
			return -EEXIST;
		}
	}

	if (first_free == -1) {
		this_cpu(gic_busy_count)++;
		return -EBUSY;
	}

	/* Inject group 0 interrupt (seen as IRQ by the guest) */
	lr = irq_id;
	lr |= GICH_LR_PENDING_BIT;
	/* Set priority to 0xa0 (Linux's default for SGIs/PPIs/SPIs).  Without
	 * this the LR carries priority 0 — highest — which masks every other
	 * vIRQ at the virtual CPU interface until vEOI.  In particular a
	 * pending vSGI 1 at vpriority 0 prevents vCNTV (PPI 27, vpriority
	 * 0xa0) from firing, freezing Linux's scheduler tick on the CPU. */
	lr |= ((0xa0 >> 3) & GICH_LR_PRIORITY_MASK) << GICH_LR_PRIORITY_SHIFT;

	if (is_sgi(irq_id)) {
		lr |= ((u32) src_cpu & 0x7) << GICH_LR_CPUID_SHIFT;
	} else {
		lr |= GICH_LR_HW_BIT;
		lr |= (u32) irq_id << GICH_LR_PHYS_ID_SHIFT;
	}

	gic_write_lr(first_free, lr);
#endif /* CONFIG_GIC_V3 */

#ifdef CONFIG_AVZ
	this_cpu(gic_inj_total_count)++;
	if (irq_id == 27)
		this_cpu(gic_inj_27_count)++;
#endif

	return 0;
}

void gic_inject_pending(void)
{
	struct pending_irqs *pirqs = &this_cpu(pending_irqs);
	u16 irq_id;

	while (pirqs->head != pirqs->tail) {
		irq_id = pirqs->irqs[pirqs->head];

		if (gic_inject_irq(irq_id) == -EBUSY) {
			gic_enable_maint_irq(true);
			return;
		}

		dmb(ish);

		pirqs->head = (pirqs->head + 1) % MAX_PENDING_IRQS;
	}

	gic_enable_maint_irq(false);
}

void gic_set_pending(u16 irq_id)
{
	struct pending_irqs *pirqs = &this_cpu(pending_irqs);
	unsigned int new_tail;

	if (gic_inject_irq(irq_id) != -EBUSY)
		return;

	spin_lock(&pirqs->lock);

	new_tail = (pirqs->tail + 1) % MAX_PENDING_IRQS;

	if (new_tail != pirqs->head) {
		pirqs->irqs[pirqs->tail] = irq_id;
		dmb(ish);
		pirqs->tail = new_tail;
	}

	spin_unlock(&pirqs->lock);

	gic_enable_maint_irq(true);
}

void gic_clear_pending_irqs(void)
{
	unsigned int n;

	/* Clear list registers. */
	for (n = 0; n < gic->gic_num_lr; n++)
		gic_write_lr(n, 0);

	/* Clear active priority bits. */
#ifdef CONFIG_GIC_V3
	write_sysreg_s(0UL, SYS_ICH_AP0R0_EL2);
	write_sysreg_s(0UL, SYS_ICH_AP1R0_EL2);
#else
	iowrite32(&gic->gich->apr, 0);
#endif
}

#endif /* CONFIG_AVZ */

#ifdef CONFIG_AVZ

#ifdef CONFIG_GIC_V3
/* GICv3 hypervisor control via ICH_* system registers. */
void gich_init(void)
{
	u32 gicc_ctlr, gicc_pmr;
	u32 vtr, vmcr;
	int n;

	/* Read physical CPU interface state via GICv3 system registers. */
	gicc_ctlr = (u32) read_sysreg(icc_ctlr_el1);
	gicc_pmr = (u32) read_sysreg(icc_pmr_el1);

	/* Reset virtual machine control register. */
	write_sysreg_s(0UL, SYS_ICH_VMCR_EL2);

	/* Clear ICH_HCR_EL2 first; EN=1 will be set after ICH_VMCR_EL2 is
	 * programmed so Linux's ICC_* accesses redirect to ICV_* virtual
	 * registers (which are valid) rather than physical registers (which are
	 * UNDEFINED at NS EL1 on this platform). */
	write_sysreg_s(0UL, SYS_ICH_HCR_EL2);

	/* Determine number of implemented list registers. */
	vtr = (u32) read_sysreg_s(SYS_ICH_VTR_EL2);
	gic->gic_num_lr = (vtr & 0x3f) + 1;

	/* ICH_VMCR_EL2 (GICv3): VPMR is 8-bit at [31:24], same encoding as ICC_PMR_EL1. */
	vmcr = (gicc_pmr & 0xff) << 24;

	/* VENG0=1 and VENG1=1: Linux's gic_has_group0() (PMR write-read test)
	 * always returns true on this platform, so Linux writes ICC_AP0R0_EL1.
	 * With EN=1 that becomes ICV_AP0R0_EL1; VENG0=1 makes that register
	 * defined.  AVZ injects all IRQs as Group 1, so virtual Group 0 is
	 * never actually delivered. */
	vmcr |= GICH_VMCR_ENABLE_GRP0_MASK | GICH_VMCR_ENABLE_GRP1_MASK;
	/* VEOIM=0: Linux's ICV_EOIR1_EL1 does priority drop + virtual deactivate.
	 * For HW=1 LRs this also triggers the physical deactivate at the
	 * redistributor, which is exactly what we want.  We do NOT mirror the
	 * physical EOImode here (EL2 set it to 1 for itself, but Linux runs in
	 * its default mode). */

	write_sysreg_s((u64) vmcr, SYS_ICH_VMCR_EL2);

	/* Enable virtual CPU interface now so Linux's ICC_* accesses during
	 * gic_cpu_sys_reg_init() are redirected to ICV_* virtual registers. */
	write_sysreg_s(GICH_HCR_EN, SYS_ICH_HCR_EL2);

	{
		struct pending_irqs *pirqs = &this_cpu(pending_irqs);
		spin_lock_init(&pirqs->lock);
		pirqs->head = 0;
		pirqs->tail = 0;
	}

	gic_clear_pending_irqs();

	for (n = 0; n < 16; n++) {
		if (ioread8(((u8 *) &gic->gicd->cpendsgirn) + n)) {
			iowrite8(((u8 *) &gic->gicd->cpendsgirn) + n, 0xff);
			gic_set_pending(n);
		}
	}
}

/* Per-CPU virtual interface init for secondary CPUs (CPU0 calls gich_init()).
 * Sets ICH_VMCR_EL2 and enables ICH_HCR_EL2.EN so Linux's ICC_* accesses
 * redirect to virtual ICV_* registers and virtual IRQs can be delivered. */
void gich_secondary_init(void)
{
	u32 gicc_ctlr = (u32) read_sysreg(icc_ctlr_el1);
	u64 vmcr;
	struct pending_irqs *pirqs = &this_cpu(pending_irqs);

	spin_lock_init(&pirqs->lock);
	pirqs->head = 0;
	pirqs->tail = 0;

	vmcr = (u64) 0xf0 << 24;
	vmcr |= GICH_VMCR_ENABLE_GRP0_MASK | GICH_VMCR_ENABLE_GRP1_MASK;
	/* VEOIM=0: see comment in gich_init(). */
	(void) gicc_ctlr;

	write_sysreg_s(vmcr, SYS_ICH_VMCR_EL2);
	isb();
	write_sysreg_s((u64) GICH_HCR_EN, SYS_ICH_HCR_EL2);
	isb();
}

/* Explicitly clear the GIC redistributor's Pending bit for a PPI (id 16-31)
 * on the current CPU, BEFORE writing EOIR.  Without this, the GIC may hold
 * Active+Pending state for a level-triggered PPI (e.g. CNTP) and EOIR would
 * transition Active+Pending → Pending instead of → Inactive, causing a storm. */
void gic_clear_ppi_pending(u16 id)
{
	int cpu_id = smp_processor_id();
	u8 *gicr_sgi = (u8 *) gic->gicc + cpu_id * 0x20000 + 0x10000;

	iowrite32(gicr_sgi + 0x280, 1u << id); /* GICR_ICPENDR0 */
	dsb(sy);
}

#else /* CONFIG_GIC_V2: hypervisor control via GICH MMIO */

void gich_init(void)
{
	struct pending_irqs *pirqs = &this_cpu(pending_irqs);
	u32 gicc_ctlr, gicc_pmr;
	u32 vtr, vmcr;
	int n;

	gicc_ctlr = ioread32(&gic->gicc->ctlr);
	gicc_pmr = ioread32(&gic->gicc->pmr);

	iowrite32(&gic->gich->vmcr, 0);

	vtr = ioread32(&gic->gich->vtr);

	/* Reveals to be 4 for virt64 board */
	gic->gic_num_lr = (vtr & 0x3f) + 1;

	/* VMCR only contains 5 bits of priority */
	vmcr = (gicc_pmr >> GICV_PMR_PRIORITY_SHIFT) << GICH_VMCR_PRIMASK_SHIFT;

	/*
         * All virtual interrupts are group 0 in this driver since the GICV
         * layout seen by the guest corresponds to GICC without security
         * extensions:
         *
         * - A read from GICV_IAR doesn't acknowledge group 1 interrupts
         *   (GICV_AIAR does it, but the guest never attempts to accesses it)
         * - A write to GICV_CTLR.GRP0EN corresponds to the GICC_CTLR.GRP1EN bit
         *   Since the guest's driver thinks that it is accessing a GIC with
         *   security extensions, a write to GPR1EN will enable group 0
         *   interrupts.
         * - Group 0 interrupts are presented as virtual IRQs (FIQEn = 0)
         */

	if (gicc_ctlr & GICC_CTLR_GRPEN1)
		vmcr |= GICH_VMCR_ENABLE_GRP0_MASK;
	if (gicc_ctlr & GICC_CTLR_EOImode)
		vmcr |= GICH_VMCR_EOI_MODE_MASK;

	iowrite32(&gic->gich->vmcr, vmcr);

	spin_lock_init(&pirqs->lock);
	pirqs->head = 0;
	pirqs->tail = 0;

	/*
         * Clear pending virtual IRQs in case anything is left from previous
         * use. Physically pending IRQs will be forwarded to Linux once we
         * enable interrupts for the hypervisor, except for SGIs, see below.
         */

	gic_clear_pending_irqs();

	iowrite32(&gic->gich->hcr, GICH_HCR_EN);

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
}

/* Per-CPU virtual interface init for secondary CPUs.
 *
 * GICH MMIO at &gic->gich and the SGI/PPI half of GICD_ISENABLER0 are banked
 * per-CPU on GICv2: each CPU sees its own VMCR/HCR/LRs and its own SGI/PPI
 * enable bits through the same address.  When a secondary CPU comes up via
 * PSCI_CPU_ON it must:
 *   - enable SGIs and PPIs at the banked GICD_ISENABLER0 (so the maintenance
 *     PPI 25 and any AVZ-internal PPIs reach this CPU),
 *   - set VMCR (Group 0 enable, priority mask mirrored from physical GICC_PMR),
 *   - enable the vGIC (HCR.EN=1),
 *   - initialise its per-CPU pending_irqs ring.
 * Without this, vIRQ delivery is off on the secondary CPU, Linux's scheduler
 * tick (PPI 27) silently disappears, and once the LR pool fills there is no
 * MAINT IRQ to drive the drain — both lead to RCU stalls. */
void gich_secondary_init(void)
{
	struct pending_irqs *pirqs = &this_cpu(pending_irqs);
	u32 gicc_ctlr = ioread32(&gic->gicc->ctlr);
	u32 gicc_pmr = ioread32(&gic->gicc->pmr);
	u32 vmcr;

	/* Enable banked SGIs/PPIs on this CPU (mirrors gic_init's CPU0 setup). */
	iowrite32(&gic->gicd->isenabler, 0xffffffff);

	spin_lock_init(&pirqs->lock);
	pirqs->head = 0;
	pirqs->tail = 0;

	vmcr = (gicc_pmr >> GICV_PMR_PRIORITY_SHIFT) << GICH_VMCR_PRIMASK_SHIFT;
	if (gicc_ctlr & GICC_CTLR_GRPEN1)
		vmcr |= GICH_VMCR_ENABLE_GRP0_MASK;
	if (gicc_ctlr & GICC_CTLR_EOImode)
		vmcr |= GICH_VMCR_EOI_MODE_MASK;

	iowrite32(&gic->gich->vmcr, vmcr);
	iowrite32(&gic->gich->hcr, GICH_HCR_EN);
}

#endif /* CONFIG_GIC_V3 */
#endif /* CONFIG_AVZ */

void gicc_init(void)
{
#ifdef CONFIG_GIC_V3
	/*
	 * GICv3: in ARE=1 mode, GICD_ISENABLER0/ICENABLER0 for SGIs/PPIs are
	 * RAZ/WI.  Per-CPU SGI/PPI configuration lives in each CPU's
	 * redistributor SGI frame (Frame 1), at GICR_base + cpu*0x20000 + 0x10000.
	 *
	 * gic->gicc maps the GICR base (reg[1] in the AVZ DTB).
	 */
	{
		int i;
		int cpu_id = smp_processor_id();
		u8 *gicr_rd = (u8 *) gic->gicc + cpu_id * 0x20000;
		u8 *gicr_sgi = gicr_rd + 0x10000;
		u32 waker;

		/* GICR_WAKER (RD_base + 0x14): clear ProcessorSleep so this
		 * redistributor forwards SGIs/PPIs to the PE. */
		waker = ioread32(gicr_rd + 0x14);
		if (waker & (1u << 1)) {
			iowrite32(gicr_rd + 0x14, waker & ~(1u << 1));
			/* Poll until ChildrenAsleep (bit 2) clears */
			do {
				waker = ioread32(gicr_rd + 0x14);
			} while (waker & (1u << 2));
		}

		/* GICR_IGROUPR0 (0x80): Group 1 NS for all SGIs/PPIs */
		iowrite32(gicr_sgi + 0x80, 0xffffffff);
		/* GICR_ICENABLER0 (0x180): disable all PPIs */
		iowrite32(gicr_sgi + 0x180, GICD_INT_EN_CLR_PPI);
		/* GICR_ICPENDR0 (0x280): clear any stale Pending state on PPIs
		 * left over from the bootloader / OPTEE / TF-A so a phantom
		 * fire (e.g. CNTV PPI 27) cannot reach Linux before its handler
		 * is registered, which would leave the LR Active forever. */
		iowrite32(gicr_sgi + 0x280, GICD_INT_EN_CLR_PPI);
		/* GICR_ISENABLER0 (0x100): enable SGIs + maintenance IRQ (PPI 25) + CNTHP (PPI 26) */
		iowrite32(gicr_sgi + 0x100, GICD_INT_EN_SET_SGI | (1u << IRQ_ARCH_ARM_MAINT) | (1u << 26u));
		/* GICR_IPRIORITYR[0..7] (0x400): priority 0 for all SGIs/PPIs */
		for (i = 0; i < 32; i += 4)
			iowrite32(gicr_sgi + 0x400 + i, 0);
	}

	/* GICv3: CPU interface via system registers (ICC_*). */
	write_sysreg(GICC_INT_PRI_THRESHOLD, icc_pmr_el1);

	/* EOImode=1 on the physical CPU interface (ICC_CTLR_EL1 from EL2 hits
	 * the physical interface): split priority drop (EOIR1) from deactivate
	 * (DIR).  Required so EL2 can drop priority after placing a forwarded
	 * IRQ in an LR (HW=1) without prematurely deactivating it — the HW=1
	 * LR triggers the physical deactivate when Linux later writes
	 * ICV_EOIR1_EL1.  AVZ-only: standalone SO3 runs at EL1 with EOImode=0
	 * (a single EOIR drops priority and deactivates), otherwise every IRQ
	 * would stay Active and block all further interrupts. */
#ifdef CONFIG_AVZ
	write_sysreg(read_sysreg(icc_ctlr_el1) | (1u << 1), icc_ctlr_el1);
	isb();
#endif

	write_sysreg_s(1UL, SYS_ICC_IGRPEN1_EL1);
#else
	{
		u32 bypass;

		/* GICv2: CPU interface via GICC MMIO. */
		iowrite32(&gic->gicc->pmr, GICC_INT_PRI_THRESHOLD);

		/*
		 * Preserve bypass disable bits to be written back later.
		 */
		bypass = ioread32(&gic->gicc->ctlr);
		bypass &= GICC_DIS_BYPASS_MASK;

		/* EOImode=1 on the physical CPU interface so EL2 can drop priority
		 * (EOIR) without deactivating, leaving HW=1 LR injections to chain
		 * the physical deactivate when the guest writes its own EOIR.
		 * AVZ-only: standalone SO3 runs at EL1 and uses EOImode=0 so a
		 * single EOIR write both drops priority and deactivates. Setting
		 * EOImode=1 here would leave every IRQ Active forever (the EL1
		 * handler never writes DIR), blocking all subsequent interrupts. */
		iowrite32(&gic->gicc->ctlr, bypass | GICC_ENABLE | GIC_CPU_EOI
#ifdef CONFIG_AVZ
						    | GICC_CTLR_EOImode
#endif
		);
	}
#endif /* CONFIG_GIC_V3 */

#ifdef CONFIG_AVZ
	gich_init();
#endif
}
static void gic_eoi_irq(u32 iar_value, bool deactivate)
{
	/*
	 * For GICv2 SGIs, EOIR/DIR writes MUST include the source CPU bits
	 * [12:10] from the IAR — otherwise the hardware leaves the SGI
	 * Active for the original source CPU, the running priority stays
	 * elevated, and no further IRQ at the same priority can fire on
	 * this CPU.  Caller passes the full IAR (or just INTID for non-SGI
	 * paths where the CPUID bits are zero anyway).
	 */
	iowrite32(&gic->gicc->eoir, iar_value);
	if (deactivate)
		iowrite32(&gic->gicc->dir, iar_value);
}

static void gic_enable(unsigned int irq)
{
	gic_unmask(irq);
}

static void gic_disable(unsigned int irq)
{
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
static void gic_handle(void *data)
{
	int irq_nr;
	int irqstat;

#ifdef CONFIG_AVZ
	this_cpu(gic_iar_count)++;
	/* Defend MAINT (PPI 25) and CNTHP (PPI 26) on this CPU.
	 * Linux's gic_cpu_init writes GICD_ICENABLER0 = 0xffff0000 on every
	 * CPU during gic init, blanket-disabling all PPIs on that CPU's
	 * banked GICD register.  Without this re-assert, Linux's secondary
	 * bring-up clobbers our hypervisor PPIs and the MAINT-driven drain
	 * of pending vIRQs (queued on -EEXIST/-EBUSY) never fires —
	 * causing RCU stalls when back-to-back SGIs arrive.  Idempotent.
	 * GICD_ISENABLER0 is banked per-CPU on GICv2 for INTIDs 0-31. */
	iowrite32(&gic->gicd->isenabler, (1u << IRQ_ARCH_ARM_MAINT) | (1u << 26));
#endif

	do {
		irqstat = ioread32(&gic->gicc->iar);
		irq_nr = irqstat & GICC_IAR_INT_ID_MASK;

		if (irq_nr > 1021)
			break;

#ifdef CONFIG_AVZ
		/* MAINT (id 25): drain overflow queue, fully deactivate. */
		if (irq_nr == IRQ_ARCH_ARM_MAINT) {
			gic_inject_pending();
			gic_eoi_irq(irqstat, true);
			continue;
		}

		/* CNTHP (id 26): AVZ's hypervisor timer.  Hardcode dispatch to
		 * avz_el2_timer_tick instead of going through irq_desc->action.
		 * If CNTHP fires before periodic_timer_init binds timer_isr (a
		 * spurious IRQ inherited from u-boot/early init), the action-NULL
		 * fallback below would inject the IRQ to the guest and never
		 * re-arm — turning AVZ's periodic timer into a one-shot and
		 * killing the EL2 scheduler tick.  Mirror the GICv3 path. */
		if (irq_nr == 26) {
			avz_el2_timer_tick();
			gic_eoi_irq(irqstat, true);
			continue;
		}

		/* SGIs (0-15): software-backed virtual IPIs. EL2 owns the physical
		 * deactivate; the LR carries a vSGI without HW=1.  Pass the full
		 * IAR (including source CPU bits[12:10]) to gic_eoi_irq so the
		 * GIC properly deactivates the SGI from its actual source.
		 * Also forward the source CPU into the LR so Linux's vIAR sees
		 * the right sender for IPI accounting. */
		if (irq_nr < 16) {
			u8 src_cpu = (irqstat >> 10) & 0x7;
			if (irq_nr == 0)
				this_cpu(gic_sgi0_recv)++;
			gic_eoi_irq(irqstat, true);
			gic_set_pending(GIC_SGI_PACK(irq_nr, src_cpu));
			continue;
		}

		/* PPIs and SPIs: dispatch by registered AVZ action; everything
		 * else (guest virt timer PPI 27, guest SPIs) is forwarded via
		 * HW=1 LR injection.  In the HW=1 case EL2 does priority drop
		 * only — the physical deactivate is chained when the guest
		 * writes its own EOIR. */
		if (irq_to_desc(irq_nr)->action != NULL) {
			irq_to_desc(irq_nr)->irq_ops->handle_high(irq_nr);
			gic_eoi_irq(irqstat, true);
		} else {
			gic_set_pending(irq_nr);
			gic_eoi_irq(irqstat, false);
		}
#else
		if (irq_nr < 16) {
			irq_to_desc(irq_nr)->irq_ops->handle_high(irq_nr);
			gic_eoi_irq(irq_nr, false);
		} else {
			irq_to_desc(irq_nr)->irq_ops->handle_high(irq_nr);
			gic_eoi_irq(irq_nr, false);
		}
#endif

	} while (true);
}

void smp_cross_call(long cpu_mask, unsigned int irq)
{
	unsigned long flags;
	int cpu = smp_processor_id();

	flags = spin_lock_irqsave(&per_cpu(intc_lock, cpu));

	smp_mb();

#ifdef CONFIG_GIC_V3
	{
		/* GICv3: GICD_SGIR is RAZ/WI when ARE=1; use ICC_SGI1R_EL1 instead.
		 * Format: [3:0]=INTID, [23:16]=Aff1 target list (one bit per CPU in cluster). */
		u64 sgi1r = ((u64) (cpu_mask & 0xffff) << 16) | (irq & 0xf);
		write_sysreg_s(sgi1r, SYS_ICC_SGI1R_EL1);
		isb();
	}
#else
	iowrite32(&gic->gicd->sgir, (cpu_mask << 16) | irq);
#endif

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

	val = oldval = ioread32(&gic->gicd->icfgr[irq / 16]);

	if (type & IRQ_TYPE_LEVEL_MASK)
		val &= ~confmask;
	else if (type & IRQ_TYPE_EDGE_BOTH)
		val |= confmask;

	/* If the current configuration is the same, then we are done */
	if (val == oldval)
		return;

	iowrite32(&gic->gicd->icfgr[irq / 16], val);
}

/**
 * @brief Reset the GIC - for instance after resuming
 * 
 */
void gic_hw_reset(void)
{
	unsigned int n;
	u32 gicd_isacter;

	iowrite32(&gic->gicd->icenabler, 0xffff0000);

	/* Deactivate all active SGIs */
	gicd_isacter = ioread32(&gic->gicd->isactiver);
	iowrite32(&gic->gicd->isactiver, gicd_isacter & 0xffff);

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
		iowrite32(&gic->gicd->itargetsr[n / 4], 0x01010101);
	}

	/* Priority for all interrupts is the highest (value 0) */
	for (n = 32; n < NR_IRQS; n += 4) {
		iowrite32(&gic->gicd->ipriorityr[n / 4], 0);
	}

	/* Disable all interrupts  */
	for (n = 32; n < NR_IRQS; n += 32) {
		iowrite32(&gic->gicd->icenabler[n / 32], 0xffffffff);
	}

	/* Enable distributor */
	iowrite32(&gic->gicd->ctlr, GICD_ENABLE);

	/* CPU interface initialization */

	/* Handle SGI (0-15) and PPI interrupts (16-31) separately */
	/* Disable all PPI and SGI interrupts */
	iowrite32(&gic->gicd->icenabler[0], 0xffffffff);

	/* Priority for all SGI and PPI interrupts is the highest (value 0) */
	for (n = 0; n < 32; n += 4) {
		iowrite32(&gic->gicd->ipriorityr[n / 4], 0);
	}

	/* Allow all priorities */
	iowrite32(&gic->gicc->pmr, 0xff);

	/* Enable CPU interface */
	iowrite32(&gic->gicc->ctlr, GICC_ENABLE);
}

/**
 * @brief We always consider using a GICv2. Initialize GICv2
 *
 * @param dev 		FDT device reference
 * @param fdt_offset 	Offset in the DTS
 * @return int
 */
static int gic_init(dev_t *dev, int fdt_offset)
{
	const struct fdt_property *prop;
	int prop_len;
	int cpu;

	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
		spin_lock_init(&per_cpu(intc_lock, cpu));
	}

	gic = (gic_t *) malloc(sizeof(gic_t));
	BUG_ON(!gic);

	LOG_DEBUG("%s\n", __FUNCTION__);

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);

#if defined(CONFIG_AVZ)
	BUG_ON(prop_len != 6 * sizeof(unsigned long));
#else
	BUG_ON(prop_len != 4 * sizeof(unsigned long));
#endif

	/* Mapping the two mem area of GIC (distributor & CPU interface) */
#ifdef CONFIG_ARCH_ARM32
	gic->gicd = (struct gicd_regs *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]),
						fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
	gic->gicd_paddr = (void *) fdt32_to_cpu(((const fdt32_t *) prop->data)[0]);

	gic->gicc = (struct gicc_regs *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[2]),
						fdt32_to_cpu(((const fdt32_t *) prop->data)[3]));
#else
	gic->gicd = (struct gicd_regs *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]),
						fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));
	gic->gicd_paddr = (void *) fdt64_to_cpu(((const fdt64_t *) prop->data)[0]);

	gic->gicc = (struct gicc_regs *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[2]),
						fdt64_to_cpu(((const fdt64_t *) prop->data)[3]));
#endif

#ifdef CONFIG_AVZ

	gic->gich = (struct gich_regs *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[4]),
						fdt64_to_cpu(((const fdt64_t *) prop->data)[5]));

	/* Enable SGIs (0-15) and PPIs (16-31), including the maintenance
	 * interrupt at PPI 25.  GICH_HCR.UIE/NPIE controls whether MAINT
	 * actually fires; the GICD enable just allows the physical PPI to
	 * reach the CPU when vGIC hardware asserts it.  Without this, when
	 * all LRs fill (4 in-flight vIRQs) gic_inject_irq returns -EBUSY
	 * and the queued IRQs in pending_irqs stay there forever — the
	 * drain via MAINT can never fire. */
	iowrite32(&gic->gicd->isenabler, 0xffffffff);

#endif

	gic_hw_reset();

	/* Initialize the CPU0 per-CPU interface and virtual CPU interface.
	 * Secondary CPUs do this in secondary_start_kernel() via gicc_init(). */
	gicc_init();

	irq_ops.enable = gic_enable;
	irq_ops.disable = gic_disable;
	irq_ops.mask = gic_mask;
	irq_ops.unmask = gic_unmask;
	irq_ops.handle_low = gic_handle;

	return 0;
}

REGISTER_DRIVER_CORE("intc,gic", gic_init);
