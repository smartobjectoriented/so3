/*
 * Copyright (C) 2026 EDGEMTech SA <info@edgemtech.ch>
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
 * IPA (Intermediate Physical Address) map for the AVZ agency on iMX8MP.
 *
 * The peripheral space (0x30000000–0x3FFFFFFF) is split into two regions so
 * that the GICv3 distributor (GICD @ 0x38800000, size 64 KiB) is NOT mapped
 * in Stage-2.  Linux accesses to GICD therefore cause Stage-2 data aborts
 * that are intercepted by AVZ's mmio_dabt_decode() handler, which forwards
 * them to the physical GICD or emulates them as needed.
 *
 * GICR (redistributor @ 0x38880000) and all other peripherals remain
 * directly accessible by Linux.
 */
ipamap_t agency_ipamap[] = {
	{
		/* AIPS1/2/3 peripherals below the GIC distributor */
		.ipa_addr = 0x30000000,
		.phys_addr = 0x30000000,
		.size = 0x08800000, /* 0x30000000 – 0x387FFFFF */
	},
	/* GICD @ 0x38800000 size 0x10000 intentionally omitted */
	{
		/* GICR, CPU interface compat, and all peripherals above GICD */
		.ipa_addr = 0x38810000,
		.phys_addr = 0x38810000,
		.size = 0x077F0000, /* 0x38810000 – 0x3FFFFFFF */
	},
};

/*
 * Capsule IPA map — minimal stub (no SOO capsules on this platform).
 * The vGIC CPU interface entry is kept for structural compatibility with
 * the hypervisor's domain setup code; the GICv3 compat GICC is mapped
 * at its physical address.
 */
ipamap_t S3C_ipamap[] = {
	{
		/* GICv3 CPU interface compatibility register frame */
		.ipa_addr = 0x38C20000,
		.phys_addr = 0x38C20000,
		.size = 0x10000,
	},
};

#endif /* MACH_IPAMAP_H */
