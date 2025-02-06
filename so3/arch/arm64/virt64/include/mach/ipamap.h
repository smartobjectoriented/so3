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

ipamap_t linux_ipamap[] = {
	{
		.ipa_addr = 0x08000000,
		.phys_addr = 0x08000000,
		.size = 0x3000000,
	},
};

/**
 * In the guest environment, the access to the GIC distributor must lead to a data abort
 * which will be trapped and handled by the hypervisor.
 */

ipamap_t guest_ipamap[] = {

	{
		/* Only mapping the CPU interface to the vGIC CPU interface.
	 * Access to the distributor must lead to a trap and be handled by the hypervisor.
	 */
		.ipa_addr = 0x08010000,
		.phys_addr = 0x08040000,
		.size = 0x10000,
	},
};

#endif /* MACH_IPAMAP_H */
