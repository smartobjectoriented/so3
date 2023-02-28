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

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/cacheflush.h>

void arch_setup_domain_frame(struct domain *d, struct cpu_regs *domain_frame, addr_t fdt_addr, addr_t start_stack, addr_t start_pc) {

	domain_frame->r2 = fdt_addr;
	domain_frame->ip = (unsigned long) d->avz_shared;

	domain_frame->sp = start_stack;
	domain_frame->pc = start_pc;

	domain_frame->psr = 0x93;  /* IRQs disabled initially */

	d->cpu_regs.sp = (unsigned long) domain_frame;
	d->cpu_regs.lr = (unsigned long) pre_ret_to_user;
}

/*
 * Setup of domain consists in setting up the 1st-level and 2nd-level page tables within the domain.
 */
void __setup_dom_pgtable(struct domain *d, addr_t v_start, unsigned long map_size, addr_t p_start) {
	u32 *new_pt;
	addr_t vaddr;

	ASSERT(d);

	/* Make sure that the size is 1 MB-aligned */
	map_size = ALIGN_UP(map_size, TTB_SECT_SIZE);

	printk("*** Setup page tables of the domain: ***\n");
	printk("   v_start          : 0x%lx\n", v_start);
	printk("   map size (bytes) : 0x%lx\n", map_size);
	printk("   phys address     : 0x%lx\n", p_start);

	/* Initial L0 page table for the domain for AArch32 */
	new_pt = (u32 *) __lva(p_start + TTB_L1_SYS_OFFSET);

	d->avz_shared->pagetable_vaddr = (addr_t) new_pt;
	d->avz_shared->pagetable_paddr = p_start + TTB_L1_SYS_OFFSET;

	/* copy page table of idle domain to guest domain */
	memcpy(new_pt, __sys_root_pgtable, TTB_L1_SIZE);

	/* Clear the area below hypervisor */
	for (vaddr = 0; vaddr < CONFIG_KERNEL_VADDR; vaddr += TTB_SECT_SIZE)
		*((uint32_t *) l1pte_offset(new_pt, vaddr)) = 0;

	/* Do the mapping of new domain at its virtual address location */
	create_mapping(new_pt, v_start, p_start, map_size, false);
}

void arch_domain_create(struct domain *d, int cpu_id) {

	if (is_idle_domain(d))
		d->avz_shared->pagetable_paddr = __pa(__sys_root_pgtable);
}

