/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#if 0
#define DEBUG
#endif

#include <memory.h>
#include <heap.h>
#include <sizes.h>
#include <string.h>

#include <device/ramdev.h>

#include <mach/uart.h>

#include <asm/mmu.h>
#include <asm/cacheflush.h>

#include <generated/autoconf.h>

void *__current_pgtable = NULL;

void *current_pgtable(void) {
	return __current_pgtable;
}

static void alloc_init_l3(u64 *l0pgtable, addr_t addr, addr_t end, addr_t phys, bool nocache)
{
	u64 *l0pte, *l1pte, *l2pte, *l3pte;
	u64 *l3pgtable;

	/* These PTEs must exist. */
	l0pte = l0pte_offset(l0pgtable, addr);
	BUG_ON(!*l0pte);

	l1pte = l1pte_offset(l0pte, addr);
	BUG_ON(!*l1pte);

	do {
		l2pte = l2pte_offset(l1pte, addr);

		/* L2 page table already exist? */
		if (!*l2pte) {

			/* A L3 table must be created */
			l3pgtable = (u64 *) memalign(TTB_L3_SIZE, PAGE_SIZE);
			BUG_ON(!l3pgtable);

			memset(l3pgtable, 0, TTB_L3_SIZE);

			/* Attach the L2 PTE to this L3 page table */
			*l2pte = __pa((addr_t) l3pgtable)  & TTB_L2_TABLE_ADDR_MASK;

			set_pte_table(l2pte, DCACHE_WRITEALLOC);

			DBG("Allocating a L3 page table at %p in l2pte: %p with contents: %lx\n", l3pgtable, l2pte, *l2pte);
		}

		l3pte = l3pte_offset(l2pte, addr);

		*l3pte = phys & TTB_L3_PAGE_ADDR_MASK;

		set_pte_page(l3pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));

		/* Set AP[1] bit 6 to 1 to make R/W/Executable the pages in user space */
		if (addr < CONFIG_KERNEL_VIRT_ADDR)
			*l3pte |= PTE_BLOCK_AP1;

		DBG("Allocating a 4 KB page at l2pte: %p content: %lx\n", l3pte, *l3pte);

		flush_pte_entry(addr, l3pte);

		phys += PAGE_SIZE;
		addr += PAGE_SIZE;


	} while (addr != end);

}

static void alloc_init_l2(u64 *l0pgtable, addr_t addr, addr_t end, addr_t phys, bool nocache)
{
	u64 *l0pte, *l1pte, *l2pte;
	u64 *l2pgtable;
	addr_t next;

	/* We are sure this pte exists */
	l0pte = l0pte_offset(l0pgtable, addr);
	BUG_ON(!*l0pte);

	do {
		l1pte = l1pte_offset(l0pte, addr);

		/* L1 page table already exist? */
		if (!*l1pte) {

			/* A L2 table must be created */
			l2pgtable = (u64 *) memalign(TTB_L2_SIZE, PAGE_SIZE);
			BUG_ON(!l2pgtable);

			memset(l2pgtable, 0, TTB_L2_SIZE);

			/* Attach the L1 PTE to this L2 page table */
			*l1pte = __pa((addr_t) l2pgtable)  & TTB_L1_TABLE_ADDR_MASK;

			set_pte_table(l1pte, DCACHE_WRITEALLOC);

			DBG("Allocating a L2 page table at %p in l1pte: %p with contents: %lx\n", l2pgtable, l1pte, *l1pte);
		}

		l2pte = l2pte_offset(l1pte, addr);

		/* Get the next address to the boundary of a 2 GB block.
		 * <addr> - <end> is <= 1 GB (L1)
		 */
		next = l2_addr_end(addr, end);

		if (((addr | next | phys) & ~BLOCK_2M_MASK) == 0) {

			*l2pte = phys & TTB_L2_BLOCK_ADDR_MASK;

			set_pte_block(l2pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));

			/* Set AP[1] bit 6 to 1 to make R/W/Executable the pages in user space */
			if (addr < CONFIG_KERNEL_VIRT_ADDR)
				*l2pte |= PTE_BLOCK_AP1;

			DBG("Allocating a 2 MB block at l2pte: %p content: %lx\n", l2pte, *l2pte);

			flush_pte_entry(addr, l2pte);

			phys += SZ_2M;
			addr += SZ_2M;

		} else {
			alloc_init_l3(l0pgtable, addr, next, phys, nocache);
			phys += next - addr;
			addr = next;
		}

	} while (addr != end);

}

static void alloc_init_l1(u64 *l0pgtable, addr_t addr, addr_t end, addr_t phys, bool nocache)
{
	u64 *l0pte, *l1pte;
	u64 *l1pgtable;
	addr_t next;

	do {
		l0pte = l0pte_offset(l0pgtable, addr);

		/* L1 page table already exist? */
		if (!*l0pte) {

			/* A L1 table must be created */
			l1pgtable = (u64 *) memalign(TTB_L1_SIZE, PAGE_SIZE);
			BUG_ON(!l1pgtable);

			memset(l1pgtable, 0, TTB_L1_SIZE);

			/* Attach the L0 PTE to this L1 page table */
			*l0pte = __pa((addr_t) l1pgtable)  & TTB_L0_TABLE_ADDR_MASK;

			set_pte_table(l0pte, DCACHE_WRITEALLOC);

			DBG("Allocating a L1 page table at %p in l0pte: %p with contents: %lx\n", l1pgtable, l0pte, *l0pte);
		}

		l1pte = l1pte_offset(l0pte, addr);

		/* Get the next address to the boundary of a 1 GB block.
		 * <addr> - <end> is <= 256 GB (L0)
		 */
		next = l1_addr_end(addr, end);

		if (((addr | next | phys) & ~BLOCK_1G_MASK) == 0) {

			*l1pte = phys & TTB_L1_BLOCK_ADDR_MASK;

			set_pte_block(l1pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));

			/* Set AP[1] bit 6 to 1 to make R/W/Executable the pages in user space */
			if (addr < CONFIG_KERNEL_VIRT_ADDR)
				*l1pte |= PTE_BLOCK_AP1;

			DBG("Allocating a 1 GB block at l1pte: %p content: %lx\n", l1pte, *l1pte);

			flush_pte_entry(addr, l1pte);

			phys += SZ_1G;
			addr += SZ_1G;

		} else {
			alloc_init_l2(l0pgtable, addr, next, phys, nocache);
			phys += next - addr;
			addr = next;
		}

	} while (addr != end);

}

/*
 * Create a static mapping between a virtual range and a physical range
 * @l0pgtable refers to the level 0 page table - if NULL, the system page table is used
 * @virt_base is the virtual address considered for this mapping
 * @phys_base is the physical address to be mapped
 * @size is the number of bytes to be mapped
 * @nocache is true if no cache (TLB) must be used (typically for I/O)
 *
 * This function tries to do the minimal mapping, i.e. using a number of page tables as low as possible, depending
 * on the granularity of mapping. In such a configuration, the function tries to map 1 GB first, then 2 MB, and finally 4 KB.
 * Mapping of blocks at L0 level is not allowed with 4 KB granule (AArch64).
 *
 */
void create_mapping(void *l0pgtable, addr_t virt_base, addr_t phys_base, size_t size, bool nocache) {
	addr_t addr, end, length, next;

	/* If l0pgtable is NULL, we consider the system page table */
	if (l0pgtable == NULL)
		l0pgtable = __sys_root_pgtable;

	BUG_ON(!size);

	DBG("Create mapping for virt %llx - phys: %llx - size: %x\n", virt_base, phys_base, size);
	addr = virt_base & PAGE_MASK;
	length = ALIGN_UP(size + (virt_base & ~PAGE_MASK), PAGE_SIZE);

	end = addr + length;

	do {
		next = l0_addr_end(addr, end);

		alloc_init_l1(l0pgtable, addr, next, phys_base, nocache);

		phys_base += next - addr;
		addr = next;

	} while (addr != end);

	mmu_page_table_flush((addr_t) l0pgtable, (addr_t) (l0pgtable + TTB_L0_ENTRIES));
}

void release_mapping(void *pgtable, addr_t virt_base, addr_t size) {

#if 0
	addr_t addr, end, length, next;
	u64 *l0pte;

	/* If l1pgtable is NULL, we consider the system page table */
	if (pgtable == NULL)
		pgtable = __sys_l1pgtable;

	addr = virt_base & PAGE_MASK;
	length = ALIGN_UP(size + (virt_base & ~PAGE_MASK), PAGE_SIZE);

	l1pte = l1pte_offset(pgtable, addr);

	end = addr + length;

	do {
		next = l1sect_addr_end(addr, end);

		free_l1_mapping(l1pte, addr, next);

		addr = next;

	} while (l1pte++, addr != end);
#endif
}

/*
 * Allocate a new page table. Return NULL if it fails.
 * The page table must be 4 KB aligned.
 */
void *new_root_pgtable(void) {
	void *pgtable;

	pgtable = memalign(TTB_L0_SIZE, PAGE_SIZE);
	if (!pgtable) {
		printk("%s: heap overflow...\n", __func__);
		kernel_panic();
	}

	/* Empty the page table */
	memset(pgtable, 0, TTB_L0_SIZE);

	return pgtable;
}

void copy_root_pgtable(void *dst, void *src) {
	memcpy(dst, src, TTB_L0_SIZE);
}


/**
 * Free a root page table and its associated Lx page tables used for the user space area.
 * We do not consider any shared pages/page tables.
 *
 * @param pgtable
 * @param remove  true if the root page table must be freed.
 */
void reset_root_pgtable(void *pgtable, bool remove) {
#if 0
	int i;
	uint32_t *l1pte, *l2pte;

	for (i = 0; i < l1pte_index(CONFIG_KERNEL_VIRT_ADDR); i++) {

		l1pte = (uint32_t *) l1pgtable + i;

		/* Check if a L2 page table is used */
		if (*l1pte) {

			if (!l1pte_is_sect(*l1pte)) {
				l2pte = (uint32_t *) __va(*l1pte & TTB_L1_PAGE_ADDR_MASK);

				free(l2pte);

				flush_pte_entry(l2pte);
			}
			*l1pte = 0;
		}
	}


	/* And finally, restore the heap memory allocated for this page table */
	if (remove)
		free(l1pgtable);

	mmu_page_table_flush((addr_t) l1pgtable, (addr_t) (l1pgtable + TTB_L1_ENTRIES));
#endif
}

/*
 * Initial configuration of system page table
 */
void dump_pgtable(void *l0pgtable) {
	u64 i, j, k, l;
	u64 *l0pte, *l1pte, *l2pte, *l3pte;
	uint64_t *__l0pgtable = (uint64_t *) l0pgtable;

	lprintk("           ***** Page table dump *****\n");

	for (i = 0; i < TTB_L0_ENTRIES; i++) {
		l0pte = __l0pgtable + i;
		if ((i != 0xe0) && *l0pte) {

			lprintk("  - L0 pte@%lx (idx %x) mapping %lx content: %lx\n", __l0pgtable+i, i, i << TTB_I0_SHIFT, *l0pte);
			BUG_ON(pte_type(l0pte) != PTE_TYPE_TABLE);

			/* Walking through the blocks/table entries */
			for (j = 0; j < TTB_L1_ENTRIES; j++) {
				l1pte = ((u64 *) __va(*l0pte & TTB_L0_TABLE_ADDR_MASK)) + j;
				if (*l1pte) {
					if (pte_type(l1pte) == PTE_TYPE_TABLE) {
						lprintk("    (TABLE) L1 pte@%lx (idx %x) mapping %lx content: %lx\n", l1pte, j,
								(i << TTB_I0_SHIFT) + (j << TTB_I1_SHIFT), *l1pte);

						for (k = 0; k < TTB_L2_ENTRIES; k++) {
							l2pte = ((u64 *) __va(*l1pte & TTB_L1_TABLE_ADDR_MASK)) + k;
							if (*l2pte) {
								if (pte_type(l2pte) == PTE_TYPE_TABLE) {
									lprintk("    (TABLE) L2 pte@%lx (idx %x) mapping %lx content: %lx\n", l2pte, k,
											(i << TTB_I0_SHIFT) + (j << TTB_I1_SHIFT) + (k << TTB_I2_SHIFT), *l2pte);

									for (l = 0; l < TTB_L3_ENTRIES; l++) {
										l3pte = ((u64 *) __va(*l2pte & TTB_L2_TABLE_ADDR_MASK)) + l;
										if (*l3pte)
											lprintk("      (PAGE) L3 pte@%lx (idx %x) mapping %lx content: %lx\n", l3pte, l,
													(i << TTB_I0_SHIFT) + (j << TTB_I1_SHIFT) + (k << TTB_I2_SHIFT) + (l << TTB_I3_SHIFT), *l3pte);
									}
								} else {
									/* Necessary of BLOCK type */
									BUG_ON(pte_type(l2pte) != PTE_TYPE_BLOCK);
									lprintk("      (PAGE) L2 pte@%lx (idx %x) mapping %lx content: %lx\n", l2pte, k,
											(i << TTB_I0_SHIFT) + (j << TTB_I1_SHIFT) + (k << TTB_I2_SHIFT), *l2pte);
								}
							}
						}
					} else {
						/* Necessary of BLOCK type */
						BUG_ON(pte_type(l1pte) != PTE_TYPE_BLOCK);

						lprintk("      (PAGE) L1 pte@%lx (idx %x) mapping %lx content: %lx\n", l1pte, j, (i << TTB_I0_SHIFT) + (j << TTB_I1_SHIFT), *l1pte);
					}
				}
			}

		}
	}
}

void mmu_configure(addr_t fdt_addr) {

	icache_disable();
	dcache_disable();

	/* Empty the page table */
	memset((void *) __sys_root_pgtable, 0, TTB_L0_SIZE);
	memset((void *) __sys_idmap_l1pgtable, 0, TTB_L1_SIZE);
	memset((void *) __sys_linearmap_l1pgtable, 0, TTB_L1_SIZE);

	/* Create an identity mapping of 1 GB on running kernel so that the kernel code can go ahead right after the MMU on */

	__sys_root_pgtable[l0pte_index(CONFIG_RAM_BASE)] = (u64) __sys_idmap_l1pgtable & TTB_L0_TABLE_ADDR_MASK;
	set_pte_table(&__sys_root_pgtable[l0pte_index(CONFIG_RAM_BASE)], DCACHE_WRITEALLOC);

	__sys_idmap_l1pgtable[l1pte_index(CONFIG_RAM_BASE)] = CONFIG_RAM_BASE & TTB_L1_BLOCK_ADDR_MASK;
	set_pte_block(&__sys_idmap_l1pgtable[l1pte_index(CONFIG_RAM_BASE)], DCACHE_WRITEALLOC);

	/* Create the initial 1 GB linear mapping of the kernel in its target virtual address space */

	__sys_root_pgtable[l0pte_index(CONFIG_KERNEL_VIRT_ADDR)] = (u64) __sys_linearmap_l1pgtable & TTB_L0_TABLE_ADDR_MASK;
	set_pte_table(&__sys_root_pgtable[l0pte_index(CONFIG_KERNEL_VIRT_ADDR)], DCACHE_WRITEALLOC);

	__sys_linearmap_l1pgtable[l1pte_index(CONFIG_KERNEL_VIRT_ADDR)] = CONFIG_RAM_BASE & TTB_L1_BLOCK_ADDR_MASK;
	set_pte_block(&__sys_linearmap_l1pgtable[l1pte_index(CONFIG_KERNEL_VIRT_ADDR)], DCACHE_WRITEALLOC);

	/* Early mapping I/O for UART. Here, the UART is supposed to be in a different L1 entry than the RAM. */

	__sys_idmap_l1pgtable[l1pte_index(UART_BASE)] = UART_BASE & TTB_L1_BLOCK_ADDR_MASK;
	set_pte_block(&__sys_idmap_l1pgtable[l1pte_index(UART_BASE)], DCACHE_OFF);

	mmu_setup(__sys_root_pgtable);

	icache_enable();
	dcache_enable();

}

#if 0
/*
 * Clear the L1 PTE used for mapping of a specific virtual address.
 */
void clear_l1pte(uint32_t *l1pgtable, uint32_t vaddr) {
	uint32_t *l1pte;

	/* If l1pgtable is NULL, we consider the system page table */
	if (l1pgtable == NULL)
		l1pgtable = __sys_l1pgtable;

	l1pte = l1pte_offset(l1pgtable, vaddr);

	*l1pte = 0;

	flush_pte_entry(l1pte);
}

#endif

/*
 * Switch the MMU to a L0 page table.
 * We *only* use ttbr1 when dealing with our hypervisor which is located in a kernel space area,
 * i.e. starting with 0xffff.... So ttbr0 is not used as soon as the id mapping in the RAM
 * is not necessary anymore.
 */
void mmu_switch(void *l0pgtable) {

	flush_dcache_all();

	__mmu_switch((void *) __pa((addr_t) l0pgtable));

	invalidate_icache_all();
	__asm_invalidate_tlb_all();

}

void duplicate_user_space(struct pcb *from, struct pcb *to) {
#if 0
	int i, j;
	u64 *l0pgtable = (u64 *) from;
	u64 *pte_origin;
	u64 *l1pgtable_origin, *l1pgtable;

	for (i = l0pte_index(CONFIG_KERNEL_VIRT_ADDR); i < TTB_L0_ENTRIES; i++) {
		pte_origin = __sys_root_pgtable + i;

		if (*pte_origin) {
			l1pgtable_origin = (u64 *) __va(*pte_origin & TTB_L0_TABLE_ADDR_MASK);

			l1pgtable = memalign(TTB_L1_SIZE, PAGE_SIZE);
			if (!l1pgtable) {
				printk("%s: heap overflow...\n", __func__);
				kernel_panic();
			}

			/* Empty the page table */
			memset(l1pgtable, 0, TTB_L1_SIZE);

			/* Copy all entries of the L1 pgtable */
			for (j = 0; j < TTB_L1_ENTRIES; j++)
				l1pgtable[j] = l1pgtable_origin[j];

			mmu_page_table_flush((addr_t) l1pgtable, (addr_t) (l1pgtable + TTB_L1_ENTRIES));

			__l0pgtable[i] = (*pte_origin & ~TTB_L0_TABLE_ADDR_MASK) | (__pa(l1pgtable) & TTB_L0_TABLE_ADDR_MASK);
		}
	}

	mmu_page_table_flush((addr_t) __l0pgtable, (addr_t) (__l0pgtable + TTB_L0_ENTRIES));
#endif

}

/* Duplicate the kernel area by doing a copy of L1 PTEs from the system page table */
void pgtable_copy_kernel_area(void *l0pgtable) {

	/* We consider to copy some key element as the ramfs for example */
	ramdev_create_mapping(l0pgtable);
}

#if 0
void dump_current_pgtable(void) {
	dump_pgtable((uint32_t *) cpu_get_l1pgtable());
}

#endif

/*
 * Get the physical address from a virtual address (valid for any virt. address).
 * The function reads the page table(s).
 */
addr_t virt_to_phys_pt(addr_t vaddr) {
	addr_t *l0pte, *l1pte, *l2pte, *l3pte;
	uint32_t offset;

	offset = vaddr & ~PAGE_MASK;

	l0pte = l0pte_offset(current_pgtable(), vaddr);
	BUG_ON(!*l0pte);

	l1pte = l1pte_offset(l0pte, vaddr);


	// To be completed...

#if 0
	addr_t *l1pte, *l2pte;
	uint32_t offset;

	/* Get the L1 PTE. */
	l1pte = l1pte_offset(current_pgtable(), vaddr);

	offset = vaddr & ~PAGE_MASK;
	BUG_ON(!*l1pte);
	if (l1pte_is_sect(*l1pte)) {

		return (*l1pte & TTB_L1_SECT_ADDR_MASK) | offset;

	} else {

		l2pte = l2pte_offset(l1pte, vaddr);

		return (*l2pte & TTB_L2_ADDR_MASK) | offset;
	}
#endif

	return 0;
}


