/*
 * Copyright (C) 2020-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

ipamap_t agency_ipamap[] = {

	/* I/O Memory space, up to the ARM_LOCAL/GIC 2 MB region.
	 * NB: entries must NOT overlap — __create_mapping cannot split an
	 * already-installed 2 MB S2 block into an L3 table (alloc_init_l3
	 * would dereference the block as a table and fault), so the GIC-400
	 * region is carved out of this window and mapped 4K-grained below. */
	{
		.ipa_addr = 0xfc000000,
		.phys_addr = 0xfc000000,
		.size = 0x03800000, /* 0xfc000000 - 0xff7fffff */
	},

	/* ARM_LOCAL block up to the GIC-400 distributor */
	{
		.ipa_addr = 0xff800000,
		.phys_addr = 0xff800000,
		.size = 0x41000, /* 0xff800000 - 0xff840fff */
	},

	/* GICD pass-through (mirroring virt64) */
	{
		.ipa_addr = 0xff841000,
		.phys_addr = 0xff841000,
		.size = 0x1000,
	},

	/* GICC view → physical GICV (vGIC CPU interface), mirroring virt64.
	 * Under the hypervisor the guest must use the virtual CPU interface,
	 * never the real GICC. GICH/GICV are EL2-only: no guest mapping at
	 * their real IPAs (accesses trap).
	 * BCM2711 GIC-400: GICC at 0xff842000, GICV at 0xff846000. */
	{
		.ipa_addr = 0xff842000,
		.phys_addr = 0xff846000,
		.size = 0x2000,
	},

	/* Rest of the GIC 2 MB region after the GIC-400 frames */
	{
		.ipa_addr = 0xff848000,
		.phys_addr = 0xff848000,
		.size = 0x1b8000, /* 0xff848000 - 0xff9fffff */
	},

	/* Remaining high peripherals */
	{
		.ipa_addr = 0xffa00000,
		.phys_addr = 0xffa00000,
		.size = 0x600000, /* 0xffa00000 - 0xffffffff */
	},

	/* VC memory*/
	{
		.ipa_addr = 0x3ea00000,
		.phys_addr = 0x3ea00000,
		.size = 0x600000,
	},

	/* HDMI 0 memory spaces */
	{
		.ipa_addr = 0x7ef00000,
		.phys_addr = 0x7ef00000,
		.size = 0x3000,
	},
	{
		.ipa_addr = 0x7ef04000,
		.phys_addr = 0x7ef04000,
		.size = 0x1000,
	},
	{
		.ipa_addr = 0x7ef20000,
		.phys_addr = 0x7ef20000,
		.size = 0x1000,
	},
	{
		.ipa_addr = 0x7e206000,
		.phys_addr = 0x7e206000,
		.size = 0x1000,
	},
	/* NB: the legacy 0xf0000000/256MB entry is gone: it fully overlapped
	 * the 0xfc000000 I/O window and the GIC carve-out above (re-installing
	 * 2 MB blocks over them), and nothing ARM-visible lives below
	 * 0xfc000000 on the BCM2711. */

	/* PCI */
	{
		.ipa_addr = 0x7d500000,
		.phys_addr = 0x7d500000,
		.size = 0x10000,
	},
	{
		.ipa_addr = 0x600000000,
		.phys_addr = 0x600000000,
		.size = 0x40000000,
	},

	/* Low page, identity. With the identity RAM layout (agency at IPA =
	 * PA 0x1000000) IPA 0 is not guest RAM, and the guest's
	 * smp_spin_table_cpu_prepare writes the spin-table release addresses
	 * (0xd8-0xf0, bcm2711 DTB) plus dcache maintenance on their line
	 * (0xc0): unmapped, those fault at S2 and panic AVZ. Identity-map the
	 * page so they land harmlessly in the vacated armstub spin area (the
	 * physical CPUs have long left it for AVZ). This mirrors the historic
	 * pre-EDGE-M1 rpi4 layout. */
	{
		.ipa_addr = 0x0,
		.phys_addr = 0x0,
		.size = 0x1000,
	},
};

/**
 * In the guest environment, the access to the GIC distributor must lead to a data abort
 * which will be trapped and handled by the hypervisor.
 */

ipamap_t S3C_ipamap[] = {

	{
		/* Only mapping the CPU interface to the vGIC CPU interface (GICV).
	 	* Access to the distributor must lead to a trap and be handled by the hypervisor.
	 	* BCM2711 GIC-400: GICV (virtual CPU interface) at 0xFF846000.
	 	*/

		.ipa_addr = 0xff842000,
		.phys_addr = 0xff846000,
		.size = 0x2000,
	},
};

#endif /* MACH_IPAMAP_H */
