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

#include <sched.h>
#include <sizes.h>
#include <types.h>

#include <asm/mmu.h>
#include <asm/processor.h>

void arch_setup_domain_frame(struct domain *d, struct cpu_regs *domain_frame, addr_t fdt_addr, addr_t start_info, addr_t start_stack, addr_t start_pc) {

	domain_frame->x21 = fdt_addr;
	domain_frame->x22 = start_info;

	domain_frame->sp = start_stack;
	domain_frame->pc = start_pc;

	d->cpu_regs.sp = (unsigned long) domain_frame;
	d->cpu_regs.lr = (unsigned long) pre_ret_to_user;
}

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
	new_pt = new_sys_pgtable();

	d->addrspace.pgtable_vaddr = (addr_t) new_pt;
	d->addrspace.pgtable_paddr = __pa(new_pt);
	d->addrspace.ttbr1[d->processor] = __pa(new_pt);

	/* Copy the hypervisor area */
	*l0pte_offset(new_pt, CONFIG_HYPERVISOR_VIRT_ADDR) = *l0pte_offset(__sys_l0pgtable, CONFIG_HYPERVISOR_VIRT_ADDR);

	/* Do the mapping of new domain at its virtual address location */
	create_mapping(new_pt, v_start, p_start, map_size, false);
}

void arch_domain_create(struct domain *d, int cpu_id) {

	if (is_idle_domain(d)) {
		d->addrspace.pgtable_paddr = __pa(__sys_l0pgtable);
		d->addrspace.pgtable_vaddr = (addr_t) __sys_l0pgtable;

		d->addrspace.ttbr1[cpu_id] = __pa(__sys_l0pgtable);
	}
}

