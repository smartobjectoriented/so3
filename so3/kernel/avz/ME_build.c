/*
 * Copyright (C) 2016-2019 Daniel Rossier <daniel.rossier@soo.tech>
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
#include <errno.h>
#include <heap.h>

#include <avz/sched.h>
#include <avz/memslot.h>
#include <avz/domain.h>

#include <asm/processor.h>
#include <asm/setup.h>
#include <asm/cacheflush.h>

#include <avz/logbool.h>

/*
 * construct_ME sets up a new Mobile Entity.
 */
int construct_ME(struct domain *d) {
	unsigned int slotID;
	unsigned long alloc_spfn;

	slotID = d->avz_shared->domID;

	printk("***************************** Loading Mobile Entity (ME) *****************************\n");

	if (memslot[slotID].size == 0)
		panic("No domU image supplied\n");

	/* We are already on the swapper_pg_dir page table to have full access to RAM */

	d->max_pages = ~0U;

	d->avz_shared->nr_pages = memslot[slotID].size >> PAGE_SHIFT;
	printk("Max dom size %d\n", memslot[slotID].size);

	printk("Domain length = %lu pages.\n", d->avz_shared->nr_pages);

	ASSERT(d);

	alloc_spfn = memslot[slotID].base_paddr >> PAGE_SHIFT;

	clear_bit(_VPF_down, &d->pause_flags);

#ifdef CONFIG_ARM64VT
	__setup_dom_pgtable(d, memslot[slotID].base_paddr, memslot[slotID].size);
#else
	__setup_dom_pgtable(d, ME_VOFFSET, memslot[slotID].size, memslot[slotID].base_paddr);
#endif

	d->avz_shared->dom_phys_offset = alloc_spfn << PAGE_SHIFT;
	d->avz_shared->hypercall_vaddr = (unsigned long) hypercall_entry;
	d->avz_shared->logbool_ht_set_addr = (unsigned long) ht_set;
	d->avz_shared->fdt_paddr = memslot[slotID].fdt_paddr;

	d->avz_shared->hypervisor_vaddr = CONFIG_KERNEL_VADDR;

	printk("ME FDT device tree: 0x%lx (phys)\n", d->avz_shared->fdt_paddr);

	d->avz_shared->printch = printch;

	/* Create the first thread associated to this domain. */

	new_thread(d, ME_VOFFSET + L_TEXT_OFFSET, d->avz_shared->fdt_paddr, ME_VOFFSET + memslot[slotID].size);

	return 0;
}

