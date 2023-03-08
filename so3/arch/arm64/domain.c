/*
 * Copyright (C) 2014-2021 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2016-2019 Baptiste Delporte <bonel@bonel.net>
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

	d->cpu_regs.sp = (unsigned long) domain_frame;
	d->cpu_regs.lr = (unsigned long) pre_ret_to_user;
}

#ifdef CONFIG_ARM64VT

/**
 * Setup the stage 2 translation page table to translate Intermediate Physical address (IPA) to to PA addresses.
 *
 * @param d
 * @param v_start
 * @param map_size
 * @param p_start
 */
void __setup_dom_pgtable(struct domain *d, addr_t paddr_start, unsigned long map_size) {
	u64 *new_pt;

	ASSERT(d);

	/* Make sure that the size is 2 MB block aligned */
	map_size = ALIGN_UP(map_size, SZ_2M);

	/* Initial L0 page table for the domain */
	new_pt = new_root_pgtable();

	printk("*** Setup page tables of the domain: ***\n");

	printk("   Real physical address    	: 0x%lx\n", paddr_start);
	printk("   Map size (bytes) 		: 0x%lx\n", map_size);

	printk("   Intermediate phys address    : 0x%lx\n", memslot[MEMSLOT_AGENCY].ipa_addr);
	printk("   Stage-2 vttbr 		: (va) 0x%lx - (pa) 0x%lx\n", new_pt, __pa(new_pt));

	d->avz_shared->pagetable_vaddr = (addr_t) new_pt;
	d->avz_shared->pagetable_paddr = __pa(new_pt);

	/* Prepare the IPA -> PA translation for this domain */
	__create_mapping(new_pt, memslot[MEMSLOT_AGENCY].ipa_addr, paddr_start, map_size, false, S2);

	do_ipamap(new_pt, ipamap, ARRAY_SIZE(ipamap));

	/* Map the shared page in the IPA space; the shared page is located right after the domain area
	 * in the IPA space.
	 */
	__create_mapping(new_pt, memslot[MEMSLOT_AGENCY].ipa_addr + map_size, __pa(d->avz_shared), PAGE_SIZE, true, S2);

	if (d->avz_shared->subdomain_shared)  {

		__create_mapping(new_pt, memslot[MEMSLOT_AGENCY].ipa_addr + map_size + PAGE_SIZE,
			       __pa(d->avz_shared->subdomain_shared_paddr), PAGE_SIZE, true, S2);

		/* <subdomain_shared_paddr> will be used by the guest only. The AGENCY_RT domain has
		 * its own shared page, so we will be able to use it via the domain descriptor in avz.
		 */
		d->avz_shared->subdomain_shared_paddr = memslot[MEMSLOT_AGENCY].ipa_addr + map_size + PAGE_SIZE;
	}
}

#else /* !CONFIG_ARM64VT */

/*
 * Setup of domain consists in setting up the 1st-level and 2nd-level page tables within the domain.
 */
void __setup_dom_pgtable(struct domain *d, addr_t v_start, unsigned long map_size, addr_t p_start) {
	u64 *new_pt;

	ASSERT(d);

	/* Make sure that the size is 2 MB block aligned */
	map_size = ALIGN_UP(map_size, SZ_2M);

	printk("*** Setup page tables of the domain: ***\n");
	printk("   v_start          : 0x%lx\n", v_start);
	printk("   map size (bytes) : 0x%lx\n", map_size);
	printk("   phys address     : 0x%lx\n", p_start);

	/* Initial L0 page table for the domain */
	new_pt = new_root_pgtable();

	d->avz_shared->pagetable_vaddr = (addr_t) new_pt;
	d->avz_shared->pagetable_paddr = __pa(new_pt);

	/* Copy the hypervisor area */
#ifdef CONFIG_VA_BITS_48
	*l0pte_offset(new_pt, CONFIG_KERNEL_VADDR) = *l0pte_offset(__sys_root_pgtable, CONFIG_KERNEL_VADDR);
#elif CONFIG_VA_BITS_39
	*(new_pt + l1pte_index(CONFIG_KERNEL_VADDR)) = *(__sys_root_pgtable + l1pte_index(CONFIG_KERNEL_VADDR));
#else
#error "Wrong VA_BITS configuration."
#endif

	/* Do the mapping of new domain at its virtual address location */
	create_mapping(new_pt, v_start, p_start, map_size, false);
}

#endif /* !CONFIG_ARM64VT */

void arch_domain_create(struct domain *d, int cpu_id) {

	if (is_idle_domain(d))
		d->avz_shared->pagetable_paddr = __pa(__sys_root_pgtable);
}

