/*
 * Copyright (C) 2016-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <common.h>
#include <percpu.h>
#include <ctype.h>
#include <console.h>
#include <memory.h>
#include <heap.h>

#include <avz/domain.h>
#include <avz/sched.h>
#include <avz/memslot.h>

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/cacheflush.h>
#include <asm/setup.h>

int construct_agency(struct domain *d)
{
	printk("***************************** Loading Guest Domain *****************************\n");

	/* Map the agency slot to the physical memory */
	create_mapping(NULL, memslot[MEMSLOT_AGENCY].base_vaddr, memslot[MEMSLOT_AGENCY].base_paddr,
		       memslot[MEMSLOT_AGENCY].size, false);

	/* Now the slot is busy. */
	memslot[MEMSLOT_AGENCY].busy = true;

	if (memslot[MEMSLOT_AGENCY].size == 0) {
		printk("No agency image supplied\n");
		kernel_panic();
	}

	d->max_pages = ~0U;

	printk("-> Agency base address from ITB: %lx\n", memslot[MEMSLOT_AGENCY].base_paddr);
	printk("-> Max dom size %ld\n", memslot[MEMSLOT_AGENCY].size);

	ASSERT(d);

	d->avz_shared->nr_pages = memslot[MEMSLOT_AGENCY].size >> PAGE_SHIFT;

	clear_bit(_VPF_down, &d->pause_flags);

	__setup_dom_pgtable(d, memslot[MEMSLOT_AGENCY].base_paddr, memslot[MEMSLOT_AGENCY].size);

	/* Propagate the virtual address of the shared info page for this domain */

	d->avz_shared->fdt_paddr = memslot[MEMSLOT_AGENCY].fdt_paddr;

	printk("AVZ Hypervisor vaddr: 0x%lx\n", CONFIG_KERNEL_VADDR);
	printk("Agency FDT device tree: 0x%lx (phys)\n", d->avz_shared->fdt_paddr);

	printk("Shared AVZ page is located at: %p\n", d->avz_shared);
	printk("Agency entry IPA: 0x%lx  FDT IPA: 0x%lx\n",
	       pa_to_ipa(MEMSLOT_AGENCY, memslot[MEMSLOT_AGENCY].entry_addr),
	       pa_to_ipa(MEMSLOT_AGENCY, d->avz_shared->fdt_paddr));

	/* Dump the first 4 instructions at the guest entry point to confirm the kernel is loaded */
	{
		u32 *insns = (u32 *) __xva(MEMSLOT_AGENCY, memslot[MEMSLOT_AGENCY].entry_addr);

		printk("Guest entry (PA 0x%lx, VA 0x%p): [0]=0x%08x [1]=0x%08x [2]=0x%08x [3]=0x%08x\n",
		       memslot[MEMSLOT_AGENCY].entry_addr, insns,
		       insns[0], insns[1], insns[2], insns[3]);
	}

	initialize_hyp_dom_stack(d, pa_to_ipa(MEMSLOT_AGENCY, d->avz_shared->fdt_paddr), pa_to_ipa(MEMSLOT_AGENCY, memslot[MEMSLOT_AGENCY].entry_addr));

	return 0;
}
