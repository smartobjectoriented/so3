
/*
 * Copyright (C) 2014-2022 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <spinlock.h>
#include <memory.h>

#include <avz/domain.h>

#include <asm/migration.h>
#include <asm/mmu.h>
#include <asm/cacheflush.h>

/**
 * Adjust the output address contained in a pte at any level
 *
 * @param pte
 * @param addr_mask
 */
static void adjust_pte(uint64_t *pte, uint64_t addr_mask) {

	uint64_t old_pfn;

	old_pfn = phys_to_pfn(*pte & addr_mask);
	*pte = (*pte & ~addr_mask) | (pfn_to_phys(old_pfn + pfn_offset) & addr_mask);
}

/**
 * fix_page_table_ME
 * Fix the ME system page table to fit new physical address space.
 *
 *
 * @param ME_slotID	slotID hosting the ME
 */
void fix_kernel_boot_page_table_ME(unsigned int ME_slotID)
{
	struct domain *me = domains[ME_slotID];
	uint64_t *pgtable_ME;
	uint64_t *l0pte, *l1pte, *l2pte, *l3pte;
	int l0, l1, l2, l3;

	/* Locate the system ME (L0) page table */
	me->avz_shared->pagetable_paddr = pfn_to_phys(phys_to_pfn(me->avz_shared->pagetable_paddr) + pfn_offset);

	pgtable_ME = (uint64_t *) __lva(me->avz_shared->pagetable_paddr);

	/* Walk through L0 page table */
	for (l0 = l0pte_index(ME_VOFFSET); l0 < TTB_L0_ENTRIES; l0++) {

		l0pte = pgtable_ME + l0;
		if (!*l0pte)
			continue;

		adjust_pte(l0pte, TTB_L0_TABLE_ADDR_MASK);

		/* Walk through L1 page table */
		for (l1 = 0; l1 < TTB_L1_ENTRIES; l1++) {

			l1pte = ((uint64_t *) __lva(*l0pte & TTB_L0_TABLE_ADDR_MASK)) + l1;

			if (!*l1pte)
				continue;

			if (pte_type(l1pte) == PTE_TYPE_BLOCK)
				adjust_pte(l1pte, TTB_L1_BLOCK_ADDR_MASK);
			else {
				adjust_pte(l1pte, TTB_L1_TABLE_ADDR_MASK);

				for (l2 = 0; l2 < TTB_L2_ENTRIES; l2++) {

					l2pte = ((uint64_t *) __lva(*l1pte & TTB_L1_TABLE_ADDR_MASK)) + l2;

					if (!*l2pte)
						continue;

					if (pte_type(l2pte) == PTE_TYPE_BLOCK)
						adjust_pte(l2pte, TTB_L2_BLOCK_ADDR_MASK);
					else {
						adjust_pte(l2pte, TTB_L2_TABLE_ADDR_MASK);

						for (l3 = 0; l3 < TTB_L3_ENTRIES; l3++) {
							l3pte = ((uint64_t *) __lva(*l2pte & TTB_L2_TABLE_ADDR_MASK)) + l3;

							if (!*l3pte)
								continue;

							adjust_pte(l3pte, TTB_L3_PAGE_ADDR_MASK);
						}
					}
				}
			}
		}
	}

	/* Fixup the hypervisor */
	*l0pte_offset(pgtable_ME, CONFIG_KERNEL_VADDR) = *l0pte_offset(__sys_root_pgtable, CONFIG_KERNEL_VADDR);
}
