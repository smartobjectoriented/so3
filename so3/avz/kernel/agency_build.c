/*
 * Copyright (C) 2016-2024 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <memory.h>
#include <heap.h>

#include <avz/domain.h>
#include <avz/sched.h>
#include <avz/memslot.h>

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/cacheflush.h>
#include <asm/setup.h>

int construct_agency(struct domain *d) {

#ifdef CONFIG_SOO
	unsigned long domain_stack;
	extern addr_t *hypervisor_stack;
	static addr_t *__hyp_stack = (unsigned long *) &hypervisor_stack;
#endif

	printk("***************************** Loading Guest Domain *****************************\n");

	/* Map the agency slot to the physical memory */
        create_mapping(NULL, memslot[MEMSLOT_AGENCY].base_vaddr, memslot[MEMSLOT_AGENCY].base_paddr, memslot[MEMSLOT_AGENCY].size, false);

	/* Now the slot is busy. */
	memslot[MEMSLOT_AGENCY].busy = true;

	if (memslot[MEMSLOT_AGENCY].size == 0) {
		printk("No agency image supplied\n");
		kernel_panic();
	}

	d->max_pages = ~0U;

	printk("-> Agency base address from ITB: %lx\n", memslot[MEMSLOT_AGENCY].base_paddr);
	printk("-> Max dom size %d\n", memslot[MEMSLOT_AGENCY].size);

	ASSERT(d);

	d->avz_shared->nr_pages = memslot[MEMSLOT_AGENCY].size >> PAGE_SHIFT;
	d->avz_shared->dom_phys_offset =  memslot[MEMSLOT_AGENCY].base_paddr;

	clear_bit(_VPF_down, &d->pause_flags);

#ifdef CONFIG_SOO

	/*
	 * Keep a reference in the primary agency domain to its sub-domain.
	 * Indeed, there is only one shared page mapped in the guest.
	 */
	agency->avz_shared->subdomain_shared = domains[DOMID_AGENCY_RT]->avz_shared;

#endif /* CONFIG_SOO */

	__setup_dom_pgtable(d, memslot[MEMSLOT_AGENCY].base_paddr, memslot[MEMSLOT_AGENCY].size);

	/* Propagate the virtual address of the shared info page for this domain */

	d->avz_shared->fdt_paddr = memslot[MEMSLOT_AGENCY].fdt_paddr;
 
	printk("AVZ Hypervisor vaddr: 0x%lx\n", CONFIG_KERNEL_VADDR);
	printk("Agency FDT device tree: 0x%lx (phys)\n", d->avz_shared->fdt_paddr);

	printk("Shared AVZ page is located at: %lx\n", d->avz_shared);

	/* HW details on the CPU: processor ID, cache ID and ARM architecture version */

	d->avz_shared->printch = printch;

#ifdef CONFIG_SOO
	/* Set up a new domain stack for the RT domain */
	domain_stack = (unsigned long) setup_dom_stack(domains[DOMID_AGENCY_RT]);

	/* Store the stack address for further needs in hypercalls/interrupt context */
	__hyp_stack[AGENCY_RT_CPU] = domain_stack;

	/* Domain related information */
	domains[DOMID_AGENCY_RT]->avz_shared->nr_pages = d->avz_shared->nr_pages;
	domains[DOMID_AGENCY_RT]->avz_shared->fdt_paddr = d->avz_shared->fdt_paddr;
	domains[DOMID_AGENCY_RT]->avz_shared->dom_phys_offset = d->avz_shared->dom_phys_offset;
	domains[DOMID_AGENCY_RT]->pagetable_paddr = d->pagetable_paddr;
	domains[DOMID_AGENCY_RT]->avz_shared->printch = d->avz_shared->printch;

#endif /* CONFIG_SOO */

	/*
	 * Create the first thread associated to this domain.
	 * The initial stack of the domain is put at the top of the domain memory area.
	 */

	new_thread(d, memslot[MEMSLOT_AGENCY].entry_addr,
		   pa_to_ipa(MEMSLOT_AGENCY, d->avz_shared->fdt_paddr),
		   memslot[MEMSLOT_AGENCY].ipa_addr + memslot[MEMSLOT_AGENCY].size);


	return 0;
}

