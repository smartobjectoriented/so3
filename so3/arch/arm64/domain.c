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
#include <heap.h>

#include <avz/sched.h>
#include <avz/memslot.h>

#include <asm/mmu.h>
#include <asm/processor.h>

#ifdef CONFIG_AVZ
#include <mach/ipamap.h>
#endif

#ifdef CONFIG_SOO
#include <avz/fbdev_gnt.h>
#endif

/**
 * @brief Initialize the content of the EL2 stack associated to this domain.
 *
 * @param d 
 * @param domain_frame 
 * @param fdt_addr 
 * @param start_stack 
 * @param start_pc 
 */
void initialize_hyp_dom_stack(struct domain *d, addr_t fdt_paddr, addr_t entry_addr)
{
	void *dom_stack;
	struct cpu_regs *frame;

	/* The stack must be aligned at STACK_SIZE bytes so that it is
	 * possible to retrieve the cpu_info structure at the bottom
	 * of the stack with a simple operation on the current stack pointer value.
	 */
	dom_stack = memalign(DOMAIN_STACK_SIZE, DOMAIN_STACK_SIZE);
	BUG_ON(!dom_stack);

	/* Keep the reference for fu16ture removal */
	d->domain_stack = dom_stack;

	/* Reserve the frame which will be restored later */
	frame = dom_stack + DOMAIN_STACK_SIZE - sizeof(cpu_regs_t);

	/* Device tree */
	frame->x0 = fdt_paddr;

	/* According to boot protocol */
	frame->x1 = 0;
	frame->x2 = 0;
	frame->x3 = 0;

	// domain_frame->sp = start_stack;
	frame->lr = entry_addr;

	/* As we will be resumed from the schedule function, we need to update the
	 * vital registers from the VCPU regs.
	 */
	d->vcpu.regs.sp = (unsigned long) frame;
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
void __setup_dom_pgtable(struct domain *d, addr_t paddr_start, unsigned long map_size)
{
	addr_t *new_pt;
	int slotID;
#ifdef CONFIG_SOO
	int i;
#endif

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
	printk("   Stage-2 vttbr 		: (va) 0x%p - (pa) 0x%lx\n", new_pt, __pa(new_pt));

	d->pagetable_vaddr = (addr_t) new_pt;
	d->pagetable_paddr = __pa(new_pt);

	/* Prepare the IPA -> PA translation for this domain */
	__create_mapping(new_pt, memslot[slotID].ipa_addr, paddr_start, map_size, false, S2);

	if (d->avz_shared->domID == DOMID_AGENCY)
		do_ipamap(new_pt, agency_ipamap, ARRAY_SIZE(agency_ipamap));
	else
		do_ipamap(new_pt, capsule_ipamap, ARRAY_SIZE(capsule_ipamap));

	/* Map the shared page in the IPA space; the shared page is located right after the domain area
	 * in the IPA space, and if any, the RT shared page follows the shared page (in IPA space).
	 */
	__create_mapping(new_pt, memslot[slotID].ipa_addr + map_size, __pa(d->avz_shared), PAGE_SIZE, false, S2);

#ifdef CONFIG_SOO
	/* Initialize the grant pfn (ipa address) area */
	for (i = 0; i < NR_GRANT_PFN; i++) {
		d->grant_pfn[i].pfn = phys_to_pfn(memslot[slotID].ipa_addr + map_size + 2 * PAGE_SIZE) + i;
		d->grant_pfn[i].free = true;
	}

	/* Set staring framebuffer ipa address after the grant pfn area */
	d->fbdev_start_pfn = phys_to_pfn(memslot[slotID].ipa_addr + map_size + 2 * PAGE_SIZE) + NR_GRANT_PFN;
	fbdev_ipamap_domain(d, slotID);
#endif /* CONFIG_SOO */
}

void arch_domain_create(struct domain *d, int cpu_id)
{
	if (is_idle_domain(d))
		d->pagetable_paddr = __pa(__sys_root_pgtable);
}
