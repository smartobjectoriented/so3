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

#include <common.h>
#include <memory.h>
#include <heap.h>
#include <sizes.h>
#include <string.h>
#include <process.h>

#include <device/ramdev.h>
#include <device/fdt.h>

#include <asm/mmu.h>
#include <asm/cacheflush.h>

#ifdef CONFIG_SO3VIRT
#include <avz/uapi/avz.h>
#endif

void *__current_pgtable = NULL;

void *current_pgtable(void) {
	return __current_pgtable;
}

/**
 * Retrieve the current physical address of the page table
 *
 * @param pgtable_paddr
 */
void mmu_get_current_pgtable(addr_t *pgtable_paddr) {
	int cpu;

	cpu = smp_processor_id();

	*pgtable_paddr = cpu_get_ttbr1();
}

static void alloc_init_l3(u64 *l0pgtable, addr_t addr, addr_t end, addr_t phys, bool nocache, mmu_stage_t stage)
{
	u64 *l1pte, *l2pte, *l3pte;
	u64 *l3pgtable;

#ifdef CONFIG_VA_BITS_48
	u64 *l0pte;

	/* These PTEs must exist. */
	l0pte = l0pte_offset(l0pgtable, addr);
	BUG_ON(!*l0pte);

	l1pte = l1pte_offset(l0pte, addr);
	BUG_ON(!*l1pte);
#elif CONFIG_VA_BITS_39
	l1pte = l0pgtable + l1pte_index(addr);
	BUG_ON(!*l1pte);
#else
#error "Wrong VA_BITS configuration."
#endif

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

#ifdef CONFIG_ARM64VT
			if (stage == S1)
				set_pte_table(l2pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
			else
				set_pte_table_S2(l2pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
#else
			set_pte_table(l2pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
#endif

			DBG("Allocating a L3 page table at %p in l2pte: %p with contents: %lx\n", l3pgtable, l2pte, *l2pte);
		}

		l3pte = l3pte_offset(l2pte, addr);

		*l3pte = phys & TTB_L3_PAGE_ADDR_MASK;

#ifdef CONFIG_ARM64VT
		if (stage == S1)
			set_pte_page(l3pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
		else
			set_pte_page_S2(l3pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
#else
		set_pte_page(l3pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));

		/* Set AP[1] bit 6 to 1 to make R/W/Executable the pages in user space */
		if ((addr != phys) && user_space_vaddr(addr))
			*l3pte |= PTE_BLOCK_AP1;
#endif

		DBG("Allocating a 4 KB page at l2pte: %p content: %lx\n", l3pte, *l3pte);

		flush_pte_entry(addr, l3pte);

		phys += PAGE_SIZE;
		addr += PAGE_SIZE;


	} while (addr != end);

}

static void alloc_init_l2(u64 *l0pgtable, addr_t addr, addr_t end, addr_t phys, bool nocache, mmu_stage_t stage)
{
	u64 *l1pte, *l2pte;
	u64 *l2pgtable;
	addr_t next;

#ifdef CONFIG_VA_BITS_48
	u64 *l0pte;

	/* We are sure this pte exists */
	l0pte = l0pte_offset(l0pgtable, addr);
	BUG_ON(!*l0pte);
#endif

	do {
#ifdef CONFIG_VA_BITS_48
		l1pte = l1pte_offset(l0pte, addr);
#elif CONFIG_VA_BITS_39
		l1pte = l0pgtable + l1pte_index(addr);
#else
#error "Wrong VA_BITS configuration."
#endif
		/* L1 page table already exist? */
		if (!*l1pte) {

			/* A L2 table must be created */
			l2pgtable = (u64 *) memalign(TTB_L2_SIZE, PAGE_SIZE);
			BUG_ON(!l2pgtable);

			memset(l2pgtable, 0, TTB_L2_SIZE);

			/* Attach the L1 PTE to this L2 page table */
			*l1pte = __pa((addr_t) l2pgtable)  & TTB_L1_TABLE_ADDR_MASK;

#ifdef CONFIG_ARM64VT
			if (stage == S1)
				set_pte_table(l1pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
			else
				set_pte_table_S2(l1pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
#else
			set_pte_table(l1pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
#endif
			DBG("Allocating a L2 page table at %p in l1pte: %p with contents: %lx\n", l2pgtable, l1pte, *l1pte);
		}

		l2pte = l2pte_offset(l1pte, addr);

		/* Get the next address to the boundary of a 2 GB block.
		 * <addr> - <end> is <= 1 GB (L1)
		 */
		next = l2_addr_end(addr, end);

		if (((addr | next | phys) & ~BLOCK_2M_MASK) == 0) {

			*l2pte = phys & TTB_L2_BLOCK_ADDR_MASK;

#ifdef CONFIG_ARM64VT
			if (stage == S1)
				set_pte_block(l2pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
			else
				set_pte_block_S2(l2pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
#else
			set_pte_block(l2pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));

			/* Set AP[1] bit 6 to 1 to make R/W/Executable the pages in user space */

			/* Warning! For an identity mapping, the RAM addresses are low compared
			 * to the kernel space. It has to be managed differently.
			 * For this reason, we compare the virt and phys address. If they are identical,
			 * we presume that the kernel is concerned by this mapping and not the user space.
			 */

			if ((addr != phys) && user_space_vaddr(addr))
				*l2pte |= PTE_BLOCK_AP1;
#endif
			DBG("Allocating a 2 MB block at l2pte: %p content: %lx\n", l2pte, *l2pte);

			flush_pte_entry(addr, l2pte);

			phys += SZ_2M;
			addr += SZ_2M;

		} else {
			alloc_init_l3(l0pgtable, addr, next, phys, nocache, stage);
			phys += next - addr;
			addr = next;
		}

	} while (addr != end);

}

#ifdef CONFIG_VA_BITS_48

static void alloc_init_l1(u64 *l0pgtable, addr_t addr, addr_t end, addr_t phys, bool nocache, mmu_stage_t stage)
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
#ifdef CONFIG_ARM64VT
			if (stage == S1)
				set_pte_table(l0pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
			else
				set_pte_table_S2(l0pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
#else
			set_pte_table(l0pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
#endif
			DBG("Allocating a L1 page table at %p in l0pte: %p with contents: %lx\n", l1pgtable, l0pte, *l0pte);
		}

		l1pte = l1pte_offset(l0pte, addr);

		/* Get the next address to the boundary of a 1 GB block.
		 * <addr> - <end> is <= 256 GB (L0)
		 */
		next = l1_addr_end(addr, end);

		if (((addr | next | phys) & ~BLOCK_1G_MASK) == 0) {

			*l1pte = phys & TTB_L1_BLOCK_ADDR_MASK;
#ifdef CONFIG_ARM64VT
			if (stage == S1)
				set_pte_block(l1pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
			else
				set_pte_block_S2(l1pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
#else
			set_pte_block(l1pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));
			
			/* Set AP[1] bit 6 to 1 to make R/W/Executable the pages in user space */
			if ((addr != phys) && user_space_vaddr(addr))
				*l1pte |= PTE_BLOCK_AP1;
#endif

			DBG("Allocating a 1 GB block at l1pte: %p content: %lx\n", l1pte, *l1pte);

			flush_pte_entry(addr, l1pte);

			phys += SZ_1G;
			addr += SZ_1G;

		} else {
			alloc_init_l2(l0pgtable, addr, next, phys, nocache, stage);
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
void __create_mapping(void *pgtable, addr_t virt_base, addr_t phys_base, size_t size, bool nocache, mmu_stage_t stage) {
	addr_t addr, end, length, next;

	/* If l0pgtable is NULL, we consider the system page table */
	if (pgtable == NULL)
		pgtable = __sys_root_pgtable;

	BUG_ON(!size);

	DBG("Create mapping for virt %llx - phys: %llx - size: %x\n", virt_base, phys_base, size);
	addr = virt_base & PAGE_MASK;
	length = ALIGN_UP(size + (virt_base & ~PAGE_MASK), PAGE_SIZE);

	end = addr + length;

	do {
		next = l0_addr_end(addr, end);

		alloc_init_l1(pgtable, addr, next, phys_base, nocache, stage);

		phys_base += next - addr;
		addr = next;

	} while (addr != end);

	mmu_page_table_flush((addr_t) pgtable, (addr_t) (pgtable + TTB_L0_SIZE));
}

void create_mapping(void *pgtable, addr_t virt_base, addr_t phys_base, size_t size, bool nocache) {
	__create_mapping(pgtable, virt_base, phys_base, size, nocache, S1);
}

#elif CONFIG_VA_BITS_39

/**
 * In the 39-bit VA version, the first page table is actually the l1pgtable from the
 * point of view of these functions.
 *
 * @param l1pgtable	The first level page table (l0pgtable actually)
 * @param virt_base
 * @param phys_base
 * @param size
 * @param nocache	true for I/O access typically
 */
void create_mapping(void *l1pgtable, addr_t virt_base, addr_t phys_base, size_t size, bool nocache) {
	addr_t addr, end, length, next, phys;
	u64 *l1pte;

	/* If l1pgtable is NULL, we consider the system page table */
	if (l1pgtable == NULL)
		l1pgtable = __sys_root_pgtable;

	BUG_ON(!size);

	addr = virt_base & PAGE_MASK;
	length = ALIGN_UP(size + (virt_base & ~PAGE_MASK), PAGE_SIZE);

	end = addr + length;
	phys = phys_base;

	do {
		next = l1_addr_end(addr, end);

		if (((addr | next | phys) & ~BLOCK_1G_MASK) == 0) {
			l1pte = l1pte_offset(l1pgtable, addr);
			*l1pte = phys & TTB_L1_BLOCK_ADDR_MASK;

			set_pte_block(l1pte, (nocache ? DCACHE_OFF : DCACHE_WRITEALLOC));

			DBG("Allocating a 1 GB block at l1pte: %p content: %lx\n", l1pte, *l1pte);

			flush_pte_entry(addr, l1pte);

			phys += SZ_1G;
			addr += SZ_1G;

		} else {
			alloc_init_l2(l1pgtable, addr, next, phys, nocache);
			phys += next - addr;
			addr = next;
		}

	} while (addr != end);

	mmu_page_table_flush((addr_t) l1pgtable, (addr_t) (l1pgtable + TTB_L1_ENTRIES));
}

#else
#error "Wrong VA_BITS configuration."
#endif

static bool empty_table(void *pgtable) {
	int i;
	uint64_t *ptr = (uint64_t *) pgtable;

	/* All levels of page table have the same number of entries */
	for (i = 0; i < TTB_L0_ENTRIES; i++)
		if (*(ptr+i))
			return false;
	/* The page table has no mapping */

	return true;
}

void release_mapping(void *pgtable, addr_t vaddr, size_t size) {
	uint64_t *l0pte, *l1pte, *l2pte, *l3pte;
	size_t free_size = 0;

	/* If l1pgtable is NULL, we consider the system page table */
	if (pgtable == NULL)
		pgtable = __sys_root_pgtable;

	vaddr = vaddr & PAGE_MASK;
	size = ALIGN_UP(size + (vaddr & ~PAGE_MASK), PAGE_SIZE);

	while (free_size < size) {
		l0pte = l0pte_offset(pgtable, vaddr);
		if (!*l0pte)
			/* Already free */
			return ;

		l1pte = l1pte_offset(l0pte, vaddr);
		BUG_ON(!*l1pte);

		if (pte_type(l1pte) == PTE_TYPE_BLOCK) {
			*l1pte = 0;
			flush_pte_entry(vaddr, l1pte);

			free_size += BLOCK_1G_OFFSET;
			vaddr += BLOCK_1G_OFFSET;

			if (empty_table(l0pte))
				free(l0pte);
		} else {
			BUG_ON(pte_type(l1pte) != PTE_TYPE_TABLE);
			l2pte = l2pte_offset(l1pte, vaddr);
			BUG_ON(!*l2pte);

			if (pte_type(l2pte) == PTE_TYPE_BLOCK) {
				*l2pte = 0;
				flush_pte_entry(vaddr, l2pte);

				free_size += BLOCK_2M_OFFSET;
				vaddr += BLOCK_2M_OFFSET;

				if (empty_table(l1pte))
					free(l1pte);
			} else {
				BUG_ON(pte_type(l2pte) != PTE_TYPE_TABLE);
				l3pte = l3pte_offset(l2pte, vaddr);
				BUG_ON(!*l3pte);

				*l3pte = 0;
				flush_pte_entry(vaddr, l3pte);

				free_size += PAGE_SIZE;
				vaddr += PAGE_SIZE;

				if (empty_table(l2pte))
					free(l2pte);
			}
		}
	}

	if (empty_table(pgtable))
		free(pgtable);
}

/*
 * Allocate a new page table. Return NULL if it fails.
 * The page table must be 4 KB aligned.
 */
void *new_root_pgtable(void) {
	void *pgtable;
	uint32_t ttb_size;

#ifdef CONFIG_VA_BITS_48
	ttb_size = TTB_L0_SIZE;
#elif CONFIG_VA_BITS_39
	ttb_size = TTB_L1_SIZE;
#else
#error "Wrong VA_BITS configuration."
#endif

	pgtable = memalign(ttb_size, PAGE_SIZE);
	if (!pgtable) {
		printk("%s: heap overflow...\n", __func__);
		kernel_panic();
	}

	/* Empty the page table */
	memset(pgtable, 0, ttb_size);

	return pgtable;
}

void copy_root_pgtable(void *dst, void *src) {
	memcpy(dst, src, TTB_L0_SIZE);

#ifdef CONFIG_SO3VIRT
	*l0pte_offset(dst, avz_shared->hypervisor_vaddr) =
		*l0pte_offset(avz_shared->pagetable_vaddr, avz_shared->hypervisor_vaddr);
#endif /* CONFIG_SO3VIRT */

}

/**
 * Free a root page table and its associated Lx page tables used for the user space area.
 * We do not consider any shared pages/page tables.
 *
 * @param pgtable
 * @param remove  true if we keep the root page table for subsequent allocations
 */
void reset_root_pgtable(void *pgtable, bool remove) {
	int i;
	u64 *pgtable_l1, *pgtable_l2, *pgtable_l3;
	u64 *l0pte, *l1pte, *l2pte;

	for (i = 0; i < TTB_L0_ENTRIES; i++) {

		l0pte = (u64 *) pgtable + i;

		/* Check if a L2 page table is used */
		if (*l0pte) {
			pgtable_l1 = (u64 *) __va(*l0pte & TTB_L0_TABLE_ADDR_MASK);

			for (i = 0; i < TTB_L1_ENTRIES; i++) {

				l1pte = pgtable_l1 + i;

				if (*l1pte && (pte_type(l1pte) == PTE_TYPE_TABLE)) {
					pgtable_l2 = (u64 *) __va(*l1pte & TTB_L1_TABLE_ADDR_MASK);

					for (i = 0; i < TTB_L2_ENTRIES; i++) {
						l2pte = pgtable_l2 + i;

						if (*l2pte && (pte_type(l2pte) == PTE_TYPE_TABLE)) {
							pgtable_l3 = (u64 *) __va(*l2pte & TTB_L2_TABLE_ADDR_MASK);

							free(pgtable_l3);
							*l2pte = 0;
						}
					}

					free(pgtable_l2);
					*l1pte = 0;
				}
			}

			free(pgtable_l1);
			*l0pte = 0;
		}
	}

	if (remove)
		free(pgtable);

	mmu_page_table_flush((addr_t) pgtable, (addr_t) (pgtable + TTB_L1_ENTRIES));

}

/*
 * Initial configuration of system page table
 */
void mmu_configure(addr_t fdt_addr) {
	int i;

	icache_disable();
	dcache_disable();

#ifdef CONFIG_AVZ
	/* The initial page table is only set by CPU #0 (AGENCY_CPU).
	 * The secondary CPUs use the same page table.
	 */

	if (smp_processor_id() == AGENCY_CPU) {
#endif
		/* Empty the page table */
		memset((void *) __sys_root_pgtable, 0, TTB_L0_SIZE);
		memset((void *) __sys_idmap_l1pgtable, 0, TTB_L1_SIZE);
		memset((void *) __sys_linearmap_l1pgtable, 0, TTB_L1_SIZE);
		memset((void *) __sys_linearmap_l2pgtable, 0, TTB_L2_SIZE);


		/* Create an identity mapping of 1 GB on running kernel so that the kernel code can go ahead right after the MMU on */
#ifdef CONFIG_VA_BITS_48
		__sys_root_pgtable[l0pte_index(CONFIG_RAM_BASE)] = (u64) __sys_idmap_l1pgtable & TTB_L0_TABLE_ADDR_MASK;
		set_pte_table(&__sys_root_pgtable[l0pte_index(CONFIG_RAM_BASE)], DCACHE_WRITEALLOC);

		__sys_idmap_l1pgtable[l1pte_index(CONFIG_RAM_BASE)] = CONFIG_RAM_BASE & TTB_L1_BLOCK_ADDR_MASK;
		set_pte_block(&__sys_idmap_l1pgtable[l1pte_index(CONFIG_RAM_BASE)], DCACHE_WRITEALLOC);

#elif CONFIG_VA_BITS_39
		__sys_root_pgtable[l1pte_index(CONFIG_RAM_BASE)] = CONFIG_RAM_BASE & TTB_L1_BLOCK_ADDR_MASK;
		set_pte_block(&__sys_root_pgtable[l1pte_index(CONFIG_RAM_BASE)], DCACHE_WRITEALLOC);
#else
#error "Wrong VA_BITS configuration."
#endif

#ifdef CONFIG_VA_BITS_48
		/* Create the initial linear mapping of the kernel in its target virtual address space */

		__sys_root_pgtable[l0pte_index(CONFIG_KERNEL_VADDR)] = (u64) __sys_linearmap_l1pgtable & TTB_L0_TABLE_ADDR_MASK;
		set_pte_table(&__sys_root_pgtable[l0pte_index(CONFIG_KERNEL_VADDR)], DCACHE_WRITEALLOC);

		__sys_linearmap_l1pgtable[l1pte_index(CONFIG_KERNEL_VADDR)] = (u64) __sys_linearmap_l2pgtable & TTB_L1_TABLE_ADDR_MASK;
		set_pte_table(&__sys_linearmap_l1pgtable[l1pte_index(CONFIG_KERNEL_VADDR)], DCACHE_WRITEALLOC);

		/* Set up a 64 MB linear mapping to progress with the bootstrap code
		 * until the memory manager re-configure the memory mapping with
		 * a better granularity.
		 */
		for (i = 0; i < 32; i++) {
			__sys_linearmap_l2pgtable[l2pte_index(CONFIG_KERNEL_VADDR + i*SZ_2M)] = (CONFIG_RAM_BASE + i*SZ_2M) & TTB_L2_BLOCK_ADDR_MASK;
			set_pte_block(&__sys_linearmap_l2pgtable[l2pte_index(CONFIG_KERNEL_VADDR + i*SZ_2M)], DCACHE_WRITEALLOC);
		}
#elif CONFIG_VA_BITS_39
		__sys_root_pgtable[l1pte_index(CONFIG_KERNEL_VADDR)] = CONFIG_RAM_BASE & TTB_L1_BLOCK_ADDR_MASK;
		set_pte_block(&__sys_root_pgtable[l1pte_index(CONFIG_KERNEL_VADDR)], DCACHE_WRITEALLOC);
#else
#error "Wrong VA_BITS configuration."
#endif

		/* Early mapping I/O for UART. Here, the UART is supposed to be in a different L1 entry than the RAM. */
#ifdef CONFIG_VA_BITS_48
		__sys_idmap_l1pgtable[l1pte_index(CONFIG_UART_LL_PADDR)] = CONFIG_UART_LL_PADDR & TTB_L1_BLOCK_ADDR_MASK;
		set_pte_block(&__sys_idmap_l1pgtable[l1pte_index(CONFIG_UART_LL_PADDR)], DCACHE_OFF);
#elif CONFIG_VA_BITS_39
		__sys_root_pgtable[l1pte_index(UART_BASE)] = CONFIG_UART_LL_PADDR & TTB_L1_BLOCK_ADDR_MASK;
		set_pte_block(&__sys_root_pgtable[l1pte_index(CONFIG_UART_LL_PADDR)], DCACHE_OFF);
#else
#error "Wrong VA_BITS configuration."
#endif

#ifdef CONFIG_AVZ
	}
#endif
	mmu_setup(__sys_root_pgtable);

	icache_enable();
	dcache_enable();

}

/*
 * Switch the MMU to a L0 page table.
 * We *only* use ttbr1 when dealing with our hypervisor which is located in a kernel space area,
 * i.e. starting with 0xffff.... So ttbr0 is not used as soon as the id mapping in the RAM
 * is not necessary anymore.
 */
void __mmu_switch_kernel(void *pgtable_paddr, bool vttbr) {

#ifdef CONFIG_SO3VIRT
	avz_shared->pagetable_paddr = (addr_t) pgtable_paddr;
#endif

	flush_dcache_all();

#ifdef CONFIG_AVZ
	if (vttbr)
		__mmu_switch_vttbr(pgtable_paddr);
	else
#endif

#ifdef CONFIG_ARM64VT
		__mmu_switch_ttbr0(pgtable_paddr);
#else
		__mmu_switch_ttbr1(pgtable_paddr);
#endif

	invalidate_icache_all();
	__asm_invalidate_tlb_all();
}

void mmu_switch_kernel(void *pgtable) {
	__mmu_switch_kernel(pgtable, false);
}

/**
 * Switch memory context in the user space range of EL1.
 *
 * @param pgtable
 */
void mmu_switch(void *pgtable_paddr) {
	flush_dcache_all();

	__mmu_switch_ttbr0(pgtable_paddr);

	invalidate_icache_all();
	__asm_invalidate_tlb_all();
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
	lprintk("**************************\n");
}

/**
 * Duplicate a highest level of page table from an entry of a lowest level page table.
 *
 * @param from	Entry which refers to a highest level page table
 * @param to	Same in the target page table
 * @param mask	Linked to the level of page table
 */
void duplicate_pgtable_entry(u64 *from, u64 *to, int level, u64 vaddr, pcb_t *pcb_to) {
	u64 *__from, *__to;
	u64 mask;
	u64 i;
	int ttb_entries, size;
	u64 paddr_to, __vaddr;

	if (level < 3) {

		switch(level) {
		case 0:
			ttb_entries = TTB_L0_ENTRIES;
			mask = TTB_L0_TABLE_ADDR_MASK;
			size = TTB_L0_SIZE;
			break;
		case 1:
			ttb_entries = TTB_L1_ENTRIES;
			mask = TTB_L1_TABLE_ADDR_MASK;
			size = TTB_L1_SIZE;
			break;
		case 2:
			ttb_entries = TTB_L2_ENTRIES;
			mask = TTB_L2_TABLE_ADDR_MASK;
			size = TTB_L2_SIZE;
			break;
		}

		for (i = 0; i < ttb_entries; i++) {

			if (from[i]) {
				__from = (u64 *) __va(from[i] & mask);

				__to = memalign(size, PAGE_SIZE);
				BUG_ON(!__to);

				memset(__to, 0, PAGE_SIZE);

				to[i] = (from[i] & ~mask) | (__pa(__to) & mask);

				switch(level) {
				case 0:
					__vaddr = i << TTB_I0_SHIFT;
					break;
				case 1:
					__vaddr = vaddr + (i << TTB_I1_SHIFT);
					break;
				case 2:
					__vaddr = vaddr + (i << TTB_I2_SHIFT);
					break;
				}

				duplicate_pgtable_entry(__from, __to, level+1, __vaddr, pcb_to);
			}
		}
		mmu_page_table_flush((addr_t) to, (addr_t) (to + ttb_entries));


	} else {

		for (i = 0; i < TTB_L3_ENTRIES; i++) {

			if (from[i]) {

				__vaddr = vaddr + (i << TTB_I3_SHIFT);

				/* Get a new free page */
				paddr_to = get_free_page();
				BUG_ON(!paddr_to);

				to[i] = (from[i] & ~TTB_L3_PAGE_ADDR_MASK) | (paddr_to & TTB_L3_PAGE_ADDR_MASK);

				/* Add the new page to the process list */
				add_page_to_proc(pcb_to, (page_t *) phys_to_page(paddr_to));

				create_mapping(NULL, FIXMAP_MAPPING, paddr_to, PAGE_SIZE, false);

				memcpy((void *) FIXMAP_MAPPING, (void *) __vaddr, PAGE_SIZE);
			}

		}
		release_mapping(current_pgtable(), FIXMAP_MAPPING, PAGE_SIZE);
	}
}

/**
 * Duplicate the user space along a fork syscall.
 * User space is mapped with 4 KB page only.
 *
 *
 * @param from	Origin L0 pagetable
 * @param to	Target pagetable of the new process
 */
void duplicate_user_space(pcb_t *from, pcb_t *to) {

	/* Walk through the L0 pgtable and copy the entries */

	duplicate_pgtable_entry((u64 *) from->pgtable, (u64 *) to->pgtable, 0, 0, to);
}

#ifdef CONFIG_RAMDEV
void ramdev_create_mapping(void *root_pgtable, addr_t ramdev_start, addr_t ramdev_end) {

	if (valid_ramdev())
		create_mapping(root_pgtable, RAMDEV_VADDR, ramdev_start, ramdev_end-ramdev_start, false);
}
#endif /* CONFIG_RAMDEV */

/**
 *
 * Get the physical address from any virtual address including
 * addresses out of the linear address space.
 *
 * We assume that <vaddr> is a real user space address if
 * the address is below the kernel space address. In other
 * words, it should never be used for kernel related virtual address.
 *
 * The function reads the page table(s).
 *
 * @param vaddr	virtual address
 * @return	physical addr corresponding to vaddr
 */
addr_t virt_to_phys_pt(addr_t vaddr) {
	addr_t *l0pte, *l1pte, *l2pte, *l3pte;
	uint32_t offset;

	offset = vaddr & ~PAGE_MASK;

	if (user_space_vaddr(vaddr))
		l0pte = l0pte_offset(current_pgtable(), vaddr);
	else
		l0pte = l0pte_offset(__sys_root_pgtable, vaddr);

	BUG_ON(!*l0pte);

	l1pte = l1pte_offset(l0pte, vaddr);
	BUG_ON(!*l1pte);

	if (pte_type(l1pte) == PTE_TYPE_BLOCK)
		return (*l1pte & TTB_L1_BLOCK_ADDR_MASK) | offset;

	BUG_ON(pte_type(l1pte) != PTE_TYPE_TABLE);

	l2pte = l2pte_offset(l1pte, vaddr);
	BUG_ON(!*l2pte);

	if (pte_type(l2pte) == PTE_TYPE_BLOCK)
		return (*l2pte & TTB_L2_BLOCK_ADDR_MASK) | offset;

	BUG_ON(pte_type(l2pte) != PTE_TYPE_TABLE);

	l3pte = l3pte_offset(l2pte, vaddr);
	BUG_ON(!*l3pte);
	BUG_ON(pte_type(l3pte) != PTE_TYPE_PAGE);

	return (*l3pte & TTB_L3_PAGE_ADDR_MASK) | offset;
}


#ifdef CONFIG_AVZ

#ifdef CONFIG_ARM64VT
/**
 * Perform a mapping of IPA regions to physical regions
 *
 * @param pgtable  	Updated page table
 * @param ipamap	List of mappings (ipa->phys)
 */
void do_ipamap(void *pgtable, ipamap_t ipamap[], int nbelement) {
	int i;

	for (i = 0; i < nbelement; i++)
		__create_mapping(pgtable, ipamap[i].ipa_addr, ipamap[i].phys_addr, ipamap[i].size, true, S2);

}

#endif /* CONFIG_AVZ */

#endif
