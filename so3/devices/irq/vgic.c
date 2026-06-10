/*
 * Copyright (C) 2024-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <mmio.h>
#include <spinlock.h>
#include <smp.h>

#include <device/arch/vgic.h>

#include <asm/io.h>
#include <asm/bitops.h>

DEFINE_SPINLOCK(dist_lock);

#ifdef CONFIG_GIC_V3

/*
 * GICv3 distributor MMIO handler.
 *
 * The physical GICD is not mapped in the guest Stage-2 tables, so every
 * Linux GICD access causes a Stage-2 data abort that lands here.  For a
 * single NS Linux guest with HCR_EL2.IMO=1 the simplest correct policy is
 * to pass all reads and writes directly to the physical GICD.  Physical SPIs
 * enabled by Linux via GICD_ISENABLER will fire, be intercepted at EL2, and
 * forwarded to Linux as virtual IRQs via ICH_LR*.
 *
 * GICD_SGIR is the GICv2-compatibility SGI register.  GICv3 Linux uses
 * ICC_SGI1R_EL1 instead, but handle it anyway for robustness.
 */
static enum mmio_result gicv3_handle_dist_access(struct mmio_access *mmio)
{
	switch (mmio->address) {
	case GICD_SGIR:
		if (mmio->is_write)
			smp_cross_call((mmio->value >> 16) & 0xff, mmio->value & 0xf);
		return MMIO_HANDLED;

	default:
		mmio_perform_access(gic->gicd, mmio);
		return MMIO_HANDLED;
	}
}

#else /* CONFIG_GIC_V2 */

/*
 * Single-guest GICv2 distributor handler — mirrors the GICv3 trap+forward
 * model.  The physical GICD is unmapped from the agency Stage-2 tables, so
 * every Linux GICD access faults here.  For a single non-secure Linux
 * guest with HCR_EL2.IMO=1, the simplest correct policy is to forward all
 * reads and writes directly to the physical GICD; SPIs Linux enables via
 * GICD_ISENABLER will fire, be intercepted at EL2, and forwarded to Linux
 * as virtual IRQs via the GICH list registers.  Special cases:
 *   - GICD_SGIR is translated into smp_cross_call so SGIs are routed via
 *     AVZ's targeted-SGI helper (and visible to the maintenance/dispatch
 *     paths) rather than going straight to the physical distributor.
 */
static enum mmio_result gicv2_handle_dist_access(struct mmio_access *mmio)
{
	switch (mmio->address) {
	case GICD_SGIR:
		if (mmio->is_write)
			smp_cross_call((mmio->value >> 16) & 0xff,
				       mmio->value & 0xf);
		return MMIO_HANDLED;

	default:
		mmio_perform_access(gic->gicd, mmio);
		return MMIO_HANDLED;
	}
}

/*
 * GICv2 uses 8bit values for each IRQ in the ITARGETSR registers
 */
static enum mmio_result gicv2_handle_irq_target(struct mmio_access *mmio, unsigned int irq)
{
	/*
         * ITARGETSR contain one byte per IRQ, so the first one affected by this
         * access corresponds to the reg index
         */
	unsigned int irq_base = irq & ~0x3;
	unsigned int offset;
	u32 access_mask = 0;
	unsigned int n;
	u8 targets;
	u32 itargetsr;

	/*
         * Let the guest freely access its SGIs and PPIs, which may be used to
         * fill its CPU interface map.
         */
	if (!is_spi(irq)) {
		mmio_perform_access(gic->gicd, mmio);
		return MMIO_HANDLED;
	}

	/*
         * The registers are byte-accessible, but we always do word accesses.
         */
	offset = irq % 4;
	mmio->address &= ~0x3;
	mmio->value <<= 8 * offset;

	for (n = offset; n < mmio->size + offset; n++) {
		access_mask |= 0xff << (8 * n);

		if (!mmio->is_write)
			continue;

		targets = (mmio->value >> (8 * n)) & 0xff;
	}

	mmio->size = 4;

	if (mmio->is_write) {
		spin_lock(&dist_lock);

		itargetsr = ioread32((void *) gic->gicd + GICD_ITARGETSR + irq_base);
		mmio->value &= access_mask;

		/* Combine with external SPIs */
		mmio->value |= (itargetsr & ~access_mask);

		/* And do the access */
		mmio_perform_access(gic->gicd, mmio);
		spin_unlock(&dist_lock);

	} else {
		mmio_perform_access(gic->gicd, mmio);
		mmio->value &= access_mask;
		mmio->value >>= 8 * offset;
	}

	return MMIO_HANDLED;
}

#endif /* CONFIG_GIC_V3 */

enum mmio_result gic_handle_dist_access(struct mmio_access *mmio)
{
#ifdef CONFIG_GIC_V3
	return gicv3_handle_dist_access(mmio);
#else
	unsigned long reg = mmio->address;
	enum mmio_result ret;

	switch (reg) {
	case REG_RANGE(GICD_IROUTER, 1024, 8):
		/* doesn't exist in v2 - ignore access */
		return MMIO_HANDLED;

	case REG_RANGE(GICD_ITARGETSR, 1024, 1):
		ret = gicv2_handle_irq_target(mmio, reg - GICD_ITARGETSR);
		break;

	case REG_RANGE(GICD_ICENABLER, 32, 4):
	case REG_RANGE(GICD_ISENABLER, 32, 4):
	case REG_RANGE(GICD_ICPENDR, 32, 4):
	case REG_RANGE(GICD_ISPENDR, 32, 4):
	case REG_RANGE(GICD_ICACTIVER, 32, 4):
	case REG_RANGE(GICD_ISACTIVER, 32, 4):
		ret = gicv2_handle_dist_access(mmio);
		break;

	default:
		ret = gicv2_handle_dist_access(mmio);
	}

	return ret;
#endif /* CONFIG_GIC_V3 */
}
