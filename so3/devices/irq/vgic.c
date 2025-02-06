/*
 * Copyright (C) 2024-2025 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

static enum mmio_result gicv2_handle_dist_access(struct mmio_access *mmio)
{
	unsigned long val = mmio->value;
	struct sgi;

	switch (mmio->address) {
	case GICD_SGIR:
		if (!mmio->is_write)
			return MMIO_HANDLED;

		smp_cross_call((val >> 16) & 0xff, val & 0xf);

		return MMIO_HANDLED;

	case GICD_CTLR:
	case GICD_TYPER:
	case GICD_IIDR:
	case REG_RANGE(GICDv2_PIDR0, 4, 4):
	case REG_RANGE(GICDv2_PIDR4, 4, 4):
	case REG_RANGE(GICDv2_CIDR0, 4, 4):
		/* Allow read access, ignore write */
		if (!mmio->is_write)
			mmio_perform_access(gic->gicd, mmio);

		/* fall through */
	default:
		/* Ignore access. */
		return MMIO_HANDLED;
	}
}

/*
 * GICv2 uses 8bit values for each IRQ in the ITARGETSR registers
 */
static enum mmio_result gicv2_handle_irq_target(struct mmio_access *mmio,
						unsigned int irq)
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

		itargetsr =
			ioread32((void *)gic->gicd + GICD_ITARGETSR + irq_base);
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

enum mmio_result gic_handle_dist_access(struct mmio_access *mmio)
{
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

		/* Currently, the guest has no way to handle a physical IRQ*/
		ret = gicv2_handle_dist_access(mmio);
		break;

	default:
		ret = gicv2_handle_dist_access(mmio);
	}

	return ret;
}