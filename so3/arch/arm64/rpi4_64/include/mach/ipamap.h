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

	/* Null pointer exception */
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

ipamap_t capsule_ipamap[] = {

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
