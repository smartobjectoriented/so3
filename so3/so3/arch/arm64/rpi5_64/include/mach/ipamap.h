/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef MACH_IPAMAP_H
#define MACH_IPAMAP_H

#include <asm/mmu.h>

/*
 * IPA (Intermediate Physical Address) map for the AVZ agency on the
 * Raspberry Pi 5 (BCM2712).
 *
 * Everything the ARM sees lives in one 2 GB window. The Linux device tree
 * puts the peripherals under a bus whose ranges are
 *
 *     ranges = <0x00000000  0x10 0x00000000  0x80000000>;
 *
 * so a node written serial@7d001000 is really at 0x10_7d001000, and the
 * whole of 0x10_00000000 .. 0x10_7fffffff is peripheral space. That single
 * window is what the entries below carve up; unlike the BCM2711, there is
 * no scattering of VideoCore and HDMI blocks across the low 4 GB to chase.
 *
 * Within it:
 *
 *   - GIC-400 (GICv2) at 0x10_7fff9000, four 4K/8K frames back to back:
 *         GICD 0x7fff9000  GICC 0x7fffa000  GICH 0x7fffc000  GICV 0x7fffe000
 *     GICD is passed through, as on virt64 -- trap-and-forward was tried
 *     there and regressed SMP bring-up. GICC is mapped onto the physical
 *     GICV so the guest's CPU-interface accesses reach the virtual
 *     interface and AVZ's list-register injections actually land. GICH and
 *     GICV are EL2-only and get no guest mapping: accesses trap.
 *
 *   - The PCIe root complexes (0x10_00100000, 0x10_00110000,
 *     0x10_00120000) fall inside the same window and are covered by the
 *     pass-through entries around the GIC.
 *
 * NOT addressed here, and the reason this platform is not claimed to work
 * on hardware yet: RP1. Every piece of board I/O on a Pi 5 -- ethernet,
 * USB, GPIO, UART, I2C -- sits behind that southbridge on the far side of
 * PCIe, and handing it to the agency needs its BAR windows and its MSI
 * path worked out at Stage-2. That is bring-up on a board, not something
 * to be guessed from a device tree.
 */

ipamap_t agency_ipamap[] = {
	{
		/* Peripheral space below the GIC block: PCIe root
		 * complexes, UART, mailbox, the lot. */
		.ipa_addr = 0x1000000000UL,
		.phys_addr = 0x1000000000UL,
		.size = 0x7fff9000UL,
	},
	{
		/* GICD pass-through. */
		.ipa_addr = 0x107fff9000UL,
		.phys_addr = 0x107fff9000UL,
		.size = 0x1000,
	},
	{
		/* GICC view -> physical GICV (vGIC CPU interface). */
		.ipa_addr = 0x107fffa000UL,
		.phys_addr = 0x107fffe000UL,
		.size = 0x2000,
	},
	{
		/* Tail of the window above the GIC frames. */
		.ipa_addr = 0x1080000000UL,
		.phys_addr = 0x1080000000UL,
		.size = 0x1000,
	},
};

/**
 * In the guest environment, the access to the GIC distributor must lead to a data abort
 * which will be trapped and handled by the hypervisor.
 */

ipamap_t S3C_ipamap[] = {
	{
		/* Only the CPU interface, mapped onto the vGIC's GICV.
		 * Distributor accesses must trap and be handled by the
		 * hypervisor. BCM2712 GIC-400: GICV at 0x10_7fffe000.
		 */

		.ipa_addr = 0x107fffa000UL,
		.phys_addr = 0x107fffe000UL,
		.size = 0x2000,
	},
};

#endif /* MACH_IPAMAP_H */
