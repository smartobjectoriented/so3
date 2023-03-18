/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright (C) 2012 ARM Limited
 *
 * Author: Will Deacon <will.deacon@arm.com>
 */

#include <errno.h>
#include <spinlock.h>
#include <smp.h>
#include <psci.h>
#include <arm-smccc.h>
#include <memory.h>

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/cacheflush.h>

#include <mach/io.h>

extern void secondary_startup(void);

/* Wake up a CPU */
void cpu_on(unsigned long cpuid, addr_t entry_point) {
	addr_t release_vaddr;

	switch (cpuid) {
	case 0:
		release_vaddr = io_map(CPU0_RELEASE_ADDR, 0x1000);
		break;

	case 1:
		release_vaddr = io_map(CPU1_RELEASE_ADDR, 0x1000);
		break;

	case 2:
		release_vaddr = io_map(CPU2_RELEASE_ADDR, 0x1000);
		break;

	case 3:
		release_vaddr = io_map(CPU3_RELEASE_ADDR, 0x1000);
		break;
	}

	*((volatile addr_t *) release_vaddr) = entry_point;

	__asm_flush_dcache_range(release_vaddr, release_vaddr + sizeof(addr_t));

	dsb(sy);

	/*
	 * Send an event to wake up the secondary CPU.
	 */
	sev();

	io_unmap(release_vaddr);
}


