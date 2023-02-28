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

ipamap_t ipamap[] = {
	{
		.ipa_addr = 0xf0000000,
		.phys_addr = 0xf0000000,
		.size = 0x10000000,
	},
	{
		.ipa_addr = 0x1faf0000,
		.phys_addr = 0x1faf0000,
		.size = 0x1000,
	},
	{
		.ipa_addr = 0x1faf1000,
		.phys_addr = 0x1faf1000,
		.size = 0x9000,
	},
	{
		.ipa_addr = 0x1fafa000,
		.phys_addr = 0x1fafa000,
		.size = 0x2000,
	},
	{
		.ipa_addr = 0x1fafc000,
		.phys_addr = 0x1fafc000,
		.size = 0x2000,
	},
	{
		.ipa_addr = 0x1fafe000,
		.phys_addr = 0x1fafe000,
		.size = 0x2000,
	},
	{
		.ipa_addr = 0x0,
		.phys_addr = 0x0,
		.size = 0x1000,
	},
	{
		.ipa_addr = 0x50000000,
		.phys_addr = 0x50000000,
		.size = 0x10000000,
	},
	{
		.ipa_addr = 0x600000000,
		.phys_addr = 0x600000000,
		.size = 0x1000,
	}
};

#endif /* MACH_IPAMAP_H */
