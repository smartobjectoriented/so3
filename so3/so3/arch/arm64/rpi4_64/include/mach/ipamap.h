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

	/* I/O Memory space*/
	{
		.ipa_addr = 0xfc000000,
		.phys_addr = 0xfc000000,
		.size = 0x04000000,
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
	{
		.ipa_addr = 0xf0000000,
		.phys_addr = 0xf0000000,
		.size = 0x10000000,
	},

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

	/* GICC view → physical GICV (vGIC CPU interface), mirroring virt64.
	 * Placed AFTER the big I/O window on purpose: do_ipamap applies the
	 * entries in order (last write wins), so this overrides the direct
	 * GICC pass-through contained in the 0xfc000000 window. The guest
	 * must never touch the real GICC under the hypervisor.
	 * BCM2711 GIC-400: GICC at 0xff842000, GICV at 0xff846000. */
	{
		.ipa_addr = 0xff842000,
		.phys_addr = 0xff846000,
		.size = 0x2000,
	},

	/* NB: no IPA 0x0 "null pointer" entry here (the old 0x10000000-based
	 * layout had one): the agency RAM now starts at IPA 0x0 and the guest
	 * entry point IS IPA 0x0 — such an entry would override the first RAM
	 * page after __setup_dom_pgtable and send the guest into the armstub
	 * area at PA 0x0 (silent WFE hang). */
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
