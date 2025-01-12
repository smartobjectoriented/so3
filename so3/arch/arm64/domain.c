/*
 * Copyright (C) 2014-2024 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <sizes.h>
#include <types.h>

#include <avz/sched.h>
#include <avz/memslot.h>

#include <asm/mmu.h>
#include <asm/processor.h>

#ifdef CONFIG_ARM64VT
#include <mach/ipamap.h>
#endif

void arch_setup_domain_frame(struct domain *d, struct cpu_regs *domain_frame, addr_t fdt_addr, addr_t start_stack, addr_t start_pc) {

	domain_frame->x21 = fdt_addr;
	domain_frame->x22 = (unsigned long) d->avz_shared;

	domain_frame->sp = start_stack;
	domain_frame->pc = start_pc;

	printk("## start_pc = %x\n", start_pc);

	d->vcpu.regs.sp = (unsigned long) domain_frame;
	d->vcpu.regs.lr = (unsigned long) pre_ret_to_user;
}


/**
 * Setup the stage 2 translation page table to translate Intermediate Physical address (IPA) to to PA addresses.
 *
 * @param d
 * @param v_start
 * @param map_size
 * @param p_start
 */
void __setup_dom_pgtable(struct domain *d, addr_t paddr_start, unsigned long map_size) {
	addr_t *new_pt;
        int slotID;
        int i;

        ASSERT(d);

	slotID = ((d->avz_shared->domID == DOMID_AGENCY) ? MEMSLOT_AGENCY : d->avz_shared->domID);

        /* Make sure that the size is 2 MB block aligned */
	map_size = ALIGN_UP(map_size, SZ_2M);

	/* Initial L0 page table for the domain */
	new_pt = new_root_pgtable();

	printk("*** Setup page tables of the domain: ***\n");

	printk("   Real physical address    	: 0x%lx\n", paddr_start);
	printk("   Map size (bytes) 		: 0x%lx\n", map_size);

	printk("   Intermediate phys address    : 0x%lx\n", memslot[slotID].ipa_addr);
        printk("   Stage-2 vttbr 		: (va) 0x%lx - (pa) 0x%lx\n", new_pt, __pa(new_pt));

        d->pagetable_vaddr = (addr_t) new_pt;
	d->pagetable_paddr = __pa(new_pt);

	/* Prepare the IPA -> PA translation for this domain */
	__create_mapping(new_pt, memslot[slotID].ipa_addr, paddr_start, map_size, false, S2);
     
        if (d->avz_shared->domID == DOMID_AGENCY)
                do_ipamap(new_pt, linux_ipamap, ARRAY_SIZE(linux_ipamap));
	else
                do_ipamap(new_pt, guest_ipamap, ARRAY_SIZE(guest_ipamap));

        /* Map the shared page in the IPA space; the shared page is located right after the domain area
	 * in the IPA space, and if any, the RT shared page follows the shared page (in IPA space).
	 */
	__create_mapping(new_pt, memslot[slotID].ipa_addr + map_size, __pa(d->avz_shared), PAGE_SIZE, true, S2);

        if (d->avz_shared->subdomain_shared)  {

		/* We map the RT domain shared page using our vaddr since it is the IPA address. */

		__create_mapping(new_pt, memslot[slotID].ipa_addr + map_size + PAGE_SIZE,
			       __pa(d->avz_shared->subdomain_shared), PAGE_SIZE, true, S2);

                /* <subdomain_shared_paddr> will be used by the guest only. The AGENCY_RT domain has
		 * its own shared page, so we will be able to use it via the domain descriptor in avz.
		 */
		d->avz_shared->subdomain_shared_paddr = memslot[slotID].ipa_addr + map_size + PAGE_SIZE;
	}

	/* Initialize the grant pfn (ipa address) area */
	for (i = 0; i < NR_GRANT_PFN; i++) {
                d->grant_pfn[i].pfn = phys_to_pfn(memslot[slotID].ipa_addr + map_size + 2 * PAGE_SIZE) + i;
		d->grant_pfn[i].free = true;
        }
}

void arch_domain_create(struct domain *d, int cpu_id) {

	if (is_idle_domain(d))
		d->pagetable_paddr = __pa(__sys_root_pgtable);
}

