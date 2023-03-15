
/*
 * Copyright (C) 2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

/* Force the variable to be stored in .data section so that the BSS can be freely cleared.
 * The value is set during the head.S execution before clear_bss().
 */

#include <memory.h>

#include <asm/mmu.h>

#include <avz/uapi/avz.h>

avz_shared_t *avz_shared = (avz_shared_t *) 0xbeef;
addr_t avz_guest_phys_offset;
void (*__printch)(char c);

volatile uint32_t *HYPERVISOR_hypercall_addr;

#ifndef CONFIG_SOO

/**
 * This function is called at early bootstrap stage along head.S.
 */
void avz_setup(void) {

	avz_guest_phys_offset = avz_shared->dom_phys_offset;
	__printch = avz_shared->printch;

	HYPERVISOR_hypercall_addr = (uint32_t *) avz_shared->hypercall_vaddr;
}

#endif /* CONFIG_SOO */
