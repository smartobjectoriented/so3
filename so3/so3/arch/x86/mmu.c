/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <memory.h>
#include <heap.h>
#include <sizes.h>
#include <string.h>
#include <process.h>

#include <device/ramdev.h>

#include <asm/mmu.h>
#include <asm/processor.h>
#include <asm/cacheflush.h>

void *__current_pgtable = NULL;

void *current_pgtable(void)
{
	return __current_pgtable;
}

static u64 *alloc_pgtable(void)
{
	u64 *pgtable;

	pgtable = (u64 *) memalign(PAGE_SIZE, PAGE_SIZE);
	if (!pgtable) {
		printk("%s: heap overflow...\n", __func__);
		kernel_panic();
	}

	memset(pgtable, 0, PAGE_SIZE);

	return pgtable;
}

/*
 * Intermediate entries grant everything: the leaf decides. User-half tables
 * carry the U/S bit so that user leaves below them are reachable.
 */

static u64 table_flags(addr_t vaddr)
{
	return PTE_PRESENT | PTE_RW | (user_space_vaddr(vaddr) ? PTE_USER : 0);
}

static u64 leaf_flags(addr_t vaddr, bool nocache)
{
	return table_flags(vaddr) | (nocache ? (PTE_PCD | PTE_PWT) : 0);
}

/* Return the next-level table of @pte, allocating it if needed */

static u64 *get_next_table(u64 *pte, addr_t vaddr)
{
	if (!*pte)
		*pte = __pa(alloc_pgtable()) | table_flags(vaddr);

	return (u64 *) __va(*pte & PTE_ADDR_MASK);
}

/*
 * New kernel L0 entries are created in the system page table; a process
 * page table which is currently loaded must see them at once.
 */

static void sync_current_kernel_area(void *pgtable)
{
	if ((pgtable == __sys_root_pgtable) && __current_pgtable && (__current_pgtable != (void *) __sys_root_pgtable))
		pgtable_copy_kernel_area(__current_pgtable);
}

/*
 * 2 MB pages are used for the kernel half when the alignment allows it.
 * User space stays page-grained, as fork() duplicates it page by page.
 */

void create_mapping(void *pgtable, addr_t virt_base, addr_t phys_base, size_t size, bool nocache)
{
	addr_t addr, end;
	u64 *l1, *l2, *l3, *pte;

	if (pgtable == NULL)
		pgtable = __sys_root_pgtable;

	BUG_ON(!size);

	addr = virt_base & PAGE_MASK;
	phys_base &= PAGE_MASK;
	end = addr + ALIGN_UP(size + (virt_base & ~PAGE_MASK), PAGE_SIZE);

	while (addr != end) {
		l1 = get_next_table(l0pte_offset(pgtable, addr), addr);
		l2 = get_next_table(l1 + l1pte_index(addr), addr);
		pte = l2 + l2pte_index(addr);

		if (!user_space_vaddr(addr) && !((addr | phys_base) & BLOCK_2M_OFFSET) && (end - addr >= SZ_2M) &&
		    (!*pte || (*pte & PTE_PS))) {
			*pte = phys_base | leaf_flags(addr, nocache) | PTE_PS;
			invlpg(addr);

			addr += SZ_2M;
			phys_base += SZ_2M;
			continue;
		}

		/* A 2 MB page being remapped with a finer granularity */

		if (*pte & PTE_PS)
			*pte = 0;

		l3 = get_next_table(pte, addr);
		l3[l3pte_index(addr)] = phys_base | leaf_flags(addr, nocache);
		invlpg(addr);

		addr += PAGE_SIZE;
		phys_base += PAGE_SIZE;
	}

	sync_current_kernel_area(pgtable);
}

static bool empty_table(u64 *table)
{
	int i;

	for (i = 0; i < TTB_L0_ENTRIES; i++)
		if (table[i])
			return false;

	return true;
}

/*
 * Tables of the kernel half are shared by every root page table: they are
 * never freed here, only the user ones are.
 */

void release_mapping(void *pgtable, addr_t vaddr, size_t size)
{
	addr_t end;
	u64 *l0e, *l1e, *l2e;
	u64 *l1, *l2, *l3;
	bool user;

	if (pgtable == NULL)
		pgtable = __sys_root_pgtable;

	end = (vaddr & PAGE_MASK) + ALIGN_UP(size + (vaddr & ~PAGE_MASK), PAGE_SIZE);
	vaddr &= PAGE_MASK;

	while (vaddr < end) {
		user = user_space_vaddr(vaddr);

		l0e = l0pte_offset(pgtable, vaddr);
		if (!*l0e) {
			vaddr = l0_addr_end(vaddr, end);
			continue;
		}

		l1 = (u64 *) __va(*l0e & PTE_ADDR_MASK);
		l1e = l1 + l1pte_index(vaddr);
		if (!*l1e) {
			vaddr = l1_addr_end(vaddr, end);
			continue;
		}

		l2 = (u64 *) __va(*l1e & PTE_ADDR_MASK);
		l2e = l2 + l2pte_index(vaddr);
		if (!*l2e) {
			vaddr = l2_addr_end(vaddr, end);
			continue;
		}

		if (*l2e & PTE_PS) {
			*l2e = 0;
			invlpg(vaddr);
			vaddr = l2_addr_end(vaddr, end);
		} else {
			l3 = (u64 *) __va(*l2e & PTE_ADDR_MASK);
			l3[l3pte_index(vaddr)] = 0;
			invlpg(vaddr);
			vaddr += PAGE_SIZE;

			if (user && empty_table(l3)) {
				*l2e = 0;
				free(l3);
			}
		}

		if (user && empty_table(l2)) {
			*l1e = 0;
			free(l2);
		}

		if (user && empty_table(l1)) {
			*l0e = 0;
			free(l1);
		}
	}
}

void *new_root_pgtable(void)
{
	return alloc_pgtable();
}

void copy_root_pgtable(void *dst, void *src)
{
	memcpy(dst, src, TTB_L0_SIZE);

	if (__pa(dst) == (read_cr3() & PTE_ADDR_MASK))
		flush_tlb_all();
}

void pgtable_copy_kernel_area(void *pgtable)
{
	memcpy((u64 *) pgtable + TTB_L0_KERNEL_FIRST, (u64 *) __sys_root_pgtable + TTB_L0_KERNEL_FIRST,
	       (TTB_L0_ENTRIES - TTB_L0_KERNEL_FIRST) * sizeof(u64));
}

static void free_user_tables(u64 *table, int level)
{
	int i;

	for (i = 0; i < TTB_L0_ENTRIES; i++) {
		if (!table[i])
			continue;

		if (level < 2 && !(table[i] & PTE_PS))
			free_user_tables((u64 *) __va(table[i] & PTE_ADDR_MASK), level + 1);
		else if (level == 2 && !(table[i] & PTE_PS))
			free((void *) __va(table[i] & PTE_ADDR_MASK));
	}

	free(table);
}

/* Drop the user half; the shared kernel half is kept */

void reset_root_pgtable(void *pgtable, bool remove)
{
	u64 *root = (u64 *) pgtable;
	int i;

	for (i = 0; i < TTB_L0_KERNEL_FIRST; i++) {
		if (root[i]) {
			free_user_tables((u64 *) __va(root[i] & PTE_ADDR_MASK), 1);
			root[i] = 0;
		}
	}

	if (__pa(pgtable) == (read_cr3() & PTE_ADDR_MASK))
		flush_tlb_all();

	if (remove)
		free(pgtable);
}

void ramdev_create_mapping(void *root_pgtable, addr_t ramdev_start, addr_t ramdev_end)
{
	if (valid_ramdev())
		create_mapping(root_pgtable, RAMDEV_VADDR, ramdev_start, ramdev_end - ramdev_start, false);
}

addr_t virt_to_phys_pt(addr_t vaddr)
{
	u64 *root, *l1e, *l2e, *l3e, *l0e;
	addr_t offset = vaddr & ~PAGE_MASK;

	root = (user_space_vaddr(vaddr) && __current_pgtable) ? (u64 *) __current_pgtable : (u64 *) __sys_root_pgtable;

	l0e = l0pte_offset(root, vaddr);
	BUG_ON(!*l0e);

	l1e = l1pte_offset(l0e, vaddr);
	BUG_ON(!*l1e);

	if (*l1e & PTE_PS)
		return (*l1e & TTB_L1_BLOCK_ADDR_MASK) | (vaddr & BLOCK_1G_OFFSET);

	l2e = l2pte_offset(l1e, vaddr);
	BUG_ON(!*l2e);

	if (*l2e & PTE_PS)
		return (*l2e & TTB_L2_BLOCK_ADDR_MASK) | (vaddr & BLOCK_2M_OFFSET);

	l3e = l3pte_offset(l2e, vaddr);
	BUG_ON(!*l3e);

	return (*l3e & TTB_L3_PAGE_ADDR_MASK) | offset;
}

/* Load a process page table, with the kernel half brought up to date */

void mmu_switch(void *pgtable_paddr)
{
	pgtable_copy_kernel_area((void *) __va(pgtable_paddr));
	write_cr3((u64) pgtable_paddr);
}

void mmu_switch_kernel(void *pgtable)
{
	write_cr3(__pa(pgtable));
}

static void duplicate_level(u64 *from, u64 *to, int level, addr_t vaddr, pcb_t *pcb_to)
{
	u64 *child;
	addr_t paddr, __vaddr;
	int i, last;

	last = (level == 0) ? TTB_L0_KERNEL_FIRST : TTB_L0_ENTRIES;

	for (i = 0; i < last; i++) {
		if (!from[i])
			continue;

		__vaddr = vaddr + ((addr_t) i << (TTB_I0_SHIFT - 9 * level));

		if (level < 3) {
			BUG_ON(from[i] & PTE_PS);

			child = alloc_pgtable();
			to[i] = (from[i] & ~PTE_ADDR_MASK) | __pa(child);

			duplicate_level((u64 *) __va(from[i] & PTE_ADDR_MASK), child, level + 1, __vaddr, pcb_to);
		} else {
			paddr = get_free_page();
			BUG_ON(!paddr);

			to[i] = (from[i] & ~PTE_ADDR_MASK) | paddr;

			add_page_to_proc(pcb_to, (page_t *) phys_to_page(paddr));

			create_mapping(NULL, FIXMAP_MAPPING, paddr, PAGE_SIZE, false);
			memcpy((void *) FIXMAP_MAPPING, (void *) __vaddr, PAGE_SIZE);
		}
	}
}

/* Copy the whole user space of @from (the current process) into @to */

void duplicate_user_space(pcb_t *from, pcb_t *to)
{
	duplicate_level((u64 *) from->pgtable, (u64 *) to->pgtable, 0, 0, to);

	release_mapping(NULL, FIXMAP_MAPPING, PAGE_SIZE);
}

void dump_pgtable(void *pgtable)
{
	u64 *root = pgtable ? (u64 *) pgtable : (u64 *) __sys_root_pgtable;
	u64 *l1, *l2;
	int i, j, k;

	lprintk("           ***** Page table dump *****\n");

	for (i = 0; i < TTB_L0_ENTRIES; i++) {
		if (!root[i])
			continue;

		lprintk("  - L0[%d] %lx\n", i, root[i]);
		l1 = (u64 *) __va(root[i] & PTE_ADDR_MASK);

		for (j = 0; j < TTB_L1_ENTRIES; j++) {
			if (!l1[j])
				continue;

			lprintk("    - L1[%d] %lx\n", j, l1[j]);
			if (l1[j] & PTE_PS)
				continue;

			l2 = (u64 *) __va(l1[j] & PTE_ADDR_MASK);
			for (k = 0; k < TTB_L2_ENTRIES; k++)
				if (l2[k])
					lprintk("      - L2[%d] %lx\n", k, l2[k]);
		}
	}
}
