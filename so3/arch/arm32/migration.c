
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
#include <types.h>

#include <avz/domain.h>

#include <asm/mmu.h>
#include <asm/migration.h>
#include <asm/cacheflush.h>

/*------------------------------------------------------------------------------
 fix_page_table_ME
 Fix ME kernel page table (swapper_pg_dir) to fit new physical address space.
 We only fix the first MBs so that kernel static mapping is fixed + the
 hypervisor mapped addresses (@ 0xFF000000) so that DOMCALLs can work.
 The rest of the page table will get fixed directly in the ME using a DOMCALL.
 ------------------------------------------------------------------------------*/
extern unsigned long vaddr_start_ME;

void fix_kernel_boot_page_table_ME(unsigned int ME_slotID)
{
	struct domain *me = domains[ME_slotID];
	uint32_t *pgtable_ME;
	unsigned long vaddr;
	unsigned long old_pfn;
	unsigned long new_pfn;
	volatile unsigned int base;
	uint32_t *l1pte, *l2pte, *l1pte_current;
	int i, j;

	/* The page table is found at domain_start + 0x4000 */
	pgtable_ME = (uint32_t *)(vaddr_start_ME + 0x4000);

	/* We re-adjust the PTE entries for the whole kernel space until the hypervisor area. */
	for (i = (AGENCY_VOFFSET >> TTB_I1_SHIFT);
	     i < (CONFIG_KERNEL_VADDR >> TTB_I1_SHIFT); i++) {
		l1pte = pgtable_ME + i;
		if (!*l1pte)
			continue;

		if (l1pte_is_sect(*l1pte)) {
			old_pfn = (*l1pte & TTB_L1_SECT_ADDR_MASK) >>
				  PAGE_SHIFT;

			new_pfn = old_pfn + pfn_offset;

			/* If we have a section PTE, it means that pfn_offset *must* be 1 MB aligned */
			BUG_ON(((new_pfn << PAGE_SHIFT) &
				~TTB_L1_SECT_ADDR_MASK) != 0);

			*l1pte = (*l1pte & ~TTB_L1_SECT_ADDR_MASK) |
				 (new_pfn << PAGE_SHIFT);

			flush_pte_entry((void *)l1pte);

		} else {
			/* Fix the pfn of the 1st-level PT */
			base = (*l1pte & TTB_L1_PAGE_ADDR_MASK);

			base += pfn_to_phys(pfn_offset);

			*l1pte = (*l1pte & ~TTB_L1_PAGE_ADDR_MASK) | base;

			flush_pte_entry((void *)l1pte);

			for (j = 0; j < 256; j++) {
				l2pte = ((uint32_t *)__lva(
						*l1pte &
						TTB_L1_PAGE_ADDR_MASK)) +
					j;
				if (*l2pte) {
					/* Re-adjust the pfn of the L2 PTE */
					base = *l2pte & PAGE_MASK;

					base += pfn_to_phys(pfn_offset);

					*l2pte = (*l2pte & ~PAGE_MASK) | base;

					flush_pte_entry((void *)l2pte);
				}
			}
		}
	}

	/* Fix the Hypervisor mapped addresses (size of hyp = 12 MB) */
	for (vaddr = 0xff000000; vaddr < 0xffc00000; vaddr += TTB_SECT_SIZE) {
		l1pte = l1pte_offset(pgtable_ME, vaddr);
		l1pte_current = l1pte_offset(__sys_root_pgtable, vaddr);

		*l1pte = *l1pte_current;
		flush_pte_entry((void *)l1pte);
	}

	/**********************/

	/* Fix the vector page physical offset */

	l1pte = pgtable_ME + (VECTOR_VADDR >> TTB_I1_SHIFT);

	/* Fix the pfn of the 1st-level PT */

	base = (*l1pte & TTB_L1_PAGE_ADDR_MASK);
	base += pfn_to_phys(pfn_offset);
	*l1pte = (*l1pte & ~TTB_L1_PAGE_ADDR_MASK) | base;

	flush_pte_entry((void *)l1pte);

	/* Walk through the 2nd-level page table */
	for (j = 0; j < 256; j++) {
		l2pte = ((uint32_t *)__lva(*l1pte & TTB_L1_PAGE_ADDR_MASK)) + j;
		if (*l2pte) {
			/* Re-adjust the pfn of the L2 PTE */
			base = *l2pte & PAGE_MASK;
			base += pfn_to_phys(pfn_offset);
			*l2pte = (*l2pte & ~PAGE_MASK) | base;

			flush_pte_entry((void *)l2pte);
		}
	}

	/**********************/

	/* Fix the physical address of the ME kernel page table */
	me->avz_shared->pagetable_paddr += pfn_to_phys(pfn_offset);
}
