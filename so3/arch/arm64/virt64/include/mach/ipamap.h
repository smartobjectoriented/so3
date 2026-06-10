/*
 * Copyright (C) 2020-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
 * IPA (Intermediate Physical Address) map for the AVZ agency on virt64.
 *
 *   - GICD  (0x08000000, size 0x10000) intentionally omitted from
 *           Stage-2: every Linux access faults to mmio_dabt_decode →
 *           gic_handle_dist_access, which forwards to physical GICD with
 *           GICD_SGIR translated into smp_cross_call.  Mirrors verdin's
 *           GICv3 model.
 *   - GICC  (guest IPA 0x08010000) → physical GICV (0x08040000) so the
 *           guest's CPU-interface accesses hit the virtual interface and
 *           AVZ's HW=1 LR injections actually reach Linux.
 *   - v2m   (0x08020000) is pass-through (Linux exposes it but does not
 *           use MSIs without a configured PCIe device).
 *   - GICH  (0x08030000) and GICV (0x08040000) are EL2-only — no guest
 *           mapping at those IPAs.
 *   - Everything else (UART, RTC, fw-cfg, virtio MMIO, …) above the GIC
 *           block stays pass-through.
 */
ipamap_t agency_ipamap[] = {
	{
		/* GICD pass-through.  Trap+forward (mirroring verdin GICv3)
		 * was attempted with two different models — direct physical
		 * GICD_SGIR replay, and Jailhouse-style queue+wake — both
		 * regress SMP bring-up under QEMU virt GICv2 + secure=on:
		 * cross-CPU SGIs queued from EL2 don't reliably wake the
		 * target Linux CPU under that combination.  Pass-through
		 * keeps Linux's GICD writes hitting hardware directly, which
		 * works correctly with the GICv2 vGIC priority/source-CPU/EOIR
		 * fixes that landed alongside this in gic.c. */
		.ipa_addr = 0x08000000,
		.phys_addr = 0x08000000,
		.size = 0x10000,
	},
	{
		/* GICC view → physical GICV (vGIC CPU interface). */
		.ipa_addr = 0x08010000,
		.phys_addr = 0x08040000,
		.size = 0x10000,
	},
	{
		/* v2m MSI frame */
		.ipa_addr = 0x08020000,
		.phys_addr = 0x08020000,
		.size = 0x10000,
	},
	{
		/* Peripherals above the GIC block (UART, RTC, virtio, …) */
		.ipa_addr = 0x08050000,
		.phys_addr = 0x08050000,
		.size = 0x2FB0000,
	},
	{
		/* PCIe MMIO low window (QEMU virt: 0x10000000-0x3effffff).
		 * Linux PCIe enumeration walks this range looking for BARs.
		 * Pass-through: no devices mean reads return 0xff and writes
		 * are harmless, but the Stage-2 mapping must exist. */
		.ipa_addr = 0x10000000,
		.phys_addr = 0x10000000,
		.size = 0x30000000,
	},
	{
		/* PCIe ECAM extended config space (QEMU virt:
		 * 0x4010000000-0x401fffffff, 256MB for buses 00-ff).  Linux's
		 * pci-host-generic accesses this region while enumerating —
		 * unmapped, the access faults at Stage-2 and crashes the
		 * faulting CPU (CPU1 in our boot log). */
		.ipa_addr = 0x4010000000UL,
		.phys_addr = 0x4010000000UL,
		.size = 0x10000000,
	},
};

/**
 * In the guest environment, the access to the GIC distributor must lead to a data abort
 * which will be trapped and handled by the hypervisor.
 */

ipamap_t capsule_ipamap[] = {

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
