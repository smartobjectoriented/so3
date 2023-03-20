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

#ifdef CONFIG_SO3VIRT
#include <avz/uapi/avz.h>
#endif

#include <device/ramdev.h>

#include <asm/mmu.h>
#include <asm/cacheflush.h>

void *__current_pgtable = NULL;

void *__sys_root_pgtable;

void *current_pgtable(void) {
	return __current_pgtable;
}

unsigned int get_ttbr0(void) {
	unsigned int ttbr0;

	asm("mrc	p15, 0, %0, c2, c0, 1	@ get ttbr0" : "=r" (ttbr0) :);

	return ttbr0;
}

unsigned int get_domain(void)
{
	unsigned int domain;

	asm("mrc	p15, 0, %0, c3, c0	@ get domain" : "=r" (domain) :);

	return domain;
}

void set_domain(uint32_t val)
{
	asm volatile(
	"mcr	p15, 0, %0, c3, c0	@ set domain" : : "r" (val) : "memory");
	isb();
}

#ifdef CONFIG_RAMDEV
void ramdev_create_mapping(void *root_pgtable, addr_t ramdev_start, addr_t ramdev_end) {

	if (valid_ramdev())
		create_mapping(root_pgtable, RAMDEV_VADDR, ramdev_start, ramdev_end-ramdev_start, false);
}
#endif /* CONFIG_RAMDEV */

/**
 * Retrieve the current physical address of the page table
 *
 * @param pgtable_paddr
 */
void mmu_get_current_pgtable(addr_t *pgtable_paddr) {
	int cpu;

	cpu = smp_processor_id();

	*pgtable_paddr = READ_CP32(TTBR0_32);
}

/* Reference to the system 1st-level page table */
static void alloc_init_pte(uint32_t *l1pte, addr_t addr, addr_t end, addr_t pfn, bool nocache)
{
	uint32_t *l2pte, *l2pgtable;
	uint32_t size;

	size = TTB_L2_ENTRIES * sizeof(uint32_t);
	
	if (!*l1pte) {
	
		l2pte = memalign(size, SZ_1K);

		ASSERT(l2pte != NULL);
		 
		memset(l2pte, 0, size);

		*l1pte =__pa((uint32_t) l2pte);

		set_l1_pte_page_dcache(l1pte, (nocache ? L1_PAGE_DCACHE_OFF : L1_PAGE_DCACHE_WRITEALLOC));

		DBG("Allocating a L2 page table at %p in l1pte: %p with contents: %x\n", l2pte, l1pte, *l1pte);

	}

	l2pgtable = (uint32_t *) __va(*l1pte & TTB_L1_PAGE_ADDR_MASK);

	l2pte = l2pte_offset(l1pte, addr);

	do {

		*l2pte = pfn << PAGE_SHIFT;

		set_l2_pte_dcache(l2pte, (nocache ? L2_DCACHE_OFF : L2_DCACHE_WRITEALLOC));

		DBG("Setting l2pte %p with contents: %x\n", l2pte, *l2pte);

		pfn++;

	} while (l2pte++, addr += PAGE_SIZE, addr != end);

	mmu_page_table_flush((uint32_t) l2pgtable, (uint32_t) (l2pgtable + TTB_L2_ENTRIES));
}

/*
 * Allocate a section (only L1 PTE) or page table (L1 & L2 page tables)
 * @nocache indicates if the page can be cache or not (true means no support for cached page)
 */
static void alloc_init_section(uint32_t *l1pte, addr_t addr, addr_t end, addr_t phys, bool nocache)
{
	/*
	 * Try a section mapping - end, addr and phys must all be aligned
	 * to a section boundary.
	 */

	if (((addr | end | phys) & ~TTB_SECT_MASK) == 0) {

		do {
			*l1pte = phys;

			set_l1_pte_sect_dcache(l1pte, (nocache ? L1_SECT_DCACHE_OFF : L1_SECT_DCACHE_WRITEALLOC));
			DBG("Allocating a section at l1pte: %p content: %x\n", l1pte, *l1pte);

			phys += TTB_SECT_SIZE;

		} while (l1pte++, addr += TTB_SECT_SIZE, addr != end);

	} else {
		/*
		 * No need to loop; L2 pte's aren't interested in the
		 * individual L1 entries.
		 */

		alloc_init_pte(l1pte, addr, end, phys >> PAGE_SHIFT, nocache);
	}
}

/*
 * Create a static mapping between a virtual range and a physical range
 *
 * @l1pgtable refers to the level 1 page table - if NULL, the system page table is used
 * @virt_base is the virtual address considered for this mapping
 * @phys_base is the physical address to be mapped
 * @size is the number of bytes to be mapped
 * @nocache is true if no cache (TLB) must be used (typically for I/O)
 */
void create_mapping(void *l1pgtable, addr_t virt_base, addr_t phys_base, uint32_t size, bool nocache) {

	addr_t addr, end, length, next;
	uint32_t *l1pte;

	/* If l1pgtable is NULL, we consider the system page table */
	if (l1pgtable == NULL)
		l1pgtable = __sys_root_pgtable;

	addr = virt_base & PAGE_MASK;
	length = ALIGN_UP(size + (virt_base & ~PAGE_MASK), PAGE_SIZE);

	l1pte = l1pte_offset((uint32_t *) l1pgtable, addr);

	end = addr + length;

	do {
		next = l1sect_addr_end(addr, end);

		alloc_init_section(l1pte, addr, next, phys_base, nocache);

		phys_base += next - addr;
		addr = next;

	} while (l1pte++, addr != end);

	/* Invalidate TLBs whenever the mapping is applied on the current page table.
	 * In other cases, the memory context switch will invalidate anyway.
	 */
	if (l1pgtable == __sys_root_pgtable)
		__asm_invalidate_tlb_all();
}

/* Empty the corresponding l2 entries */
static void free_l2_mapping(uint32_t *l1pte, addr_t addr, addr_t end) {
	uint32_t *l2pte, *pgtable;
	int i;
	bool found;

	pgtable = l2pte_first(l1pte);

	l2pte = l2pte_offset(l1pte, addr);

	do {
		DBG("Re-setting l2pte to 0: %p\n", l2pte);

		*l2pte = 0; /* Free this entry */

	} while (l2pte++, addr += PAGE_SIZE, addr != end);

	mmu_page_table_flush((uint32_t) pgtable, (uint32_t) (pgtable + TTB_L2_ENTRIES));

	for (i = 0, found = false, l2pte = l2pte_first(l1pte); !found && (i < TTB_L2_ENTRIES); i++)
		found = (*(l2pte + i) != 0);

	if (!found) {

		free(l2pte); /* Remove the L2 page table since all no entry is mapped */

		*l1pte = 0; /* Free the L1 entry as well */

		flush_pte_entry(l1pte);
	}

}

/* Empty the corresponding l1 entries */
static void free_l1_mapping(uint32_t *l1pte, addr_t addr, addr_t end) {
	uint32_t *__l1pte = l1pte;

	/*
	 * Try a section mapping - end, addr and phys must all be aligned
	 * to a section boundary.
	 */
	if (((addr | end) & ~TTB_SECT_MASK) == 0) {

		do {
			DBG("Re-setting l1pte: %p to 0\n", l1pte);

			*l1pte = 0; /* Free this entry */

		} while (l1pte++, addr += TTB_SECT_SIZE, addr != end);

		mmu_page_table_flush((uint32_t) __l1pte, (uint32_t) l1pte);

	} else {
		/*
		 * No need to loop; L2 pte's aren't interested in the
		 * individual L1 entries.
		 */
		free_l2_mapping(l1pte, addr, end);
	}
}

/*
 * Release an existing mapping
 */
void release_mapping(void *pgtable, addr_t virt_base, uint32_t size) {
	addr_t addr, end, length, next;
	uint32_t *l1pte;

	/* If pgtable is NULL, we consider the system page table */
	if (pgtable == NULL)
		pgtable = __sys_root_pgtable;

	addr = virt_base & PAGE_MASK;
	length = ALIGN_UP(size + (virt_base & ~PAGE_MASK), PAGE_SIZE);

	l1pte = l1pte_offset((uint32_t *) pgtable, addr);

	end = addr + length;

	do {
		next = l1sect_addr_end(addr, end);

		free_l1_mapping(l1pte, addr, next);

		addr = next;

	} while (l1pte++, addr != end);
}

/*
 * Initial configuration of system page table
 * MMU is off
 */
void mmu_configure(addr_t l1pgtable, addr_t fdt_addr) {

#ifndef CONFIG_SO3VIRT
	unsigned int i;
#endif /* CONFIG_SO3VIRT */

	uint32_t *__pgtable = (uint32_t *) l1pgtable;

#ifndef CONFIG_SO3VIRT

	icache_disable();
	dcache_disable();

#ifdef CONFIG_AVZ
	if (smp_processor_id() == AGENCY_CPU) {
#endif /* CONFIG_AVZ */

	/* Empty the page table */
	for (i = 0; i < 4096; i++)
		__pgtable[i] = 0;

	/*
	 * The kernel mapping has to be done with "normal memory" attribute, i.e. using cacheable mappings.
	 * This is required for the use of ldrex/strex instructions in recent core such as Cortex-A72 (or armv8 in general).
	 * Otherwise, strex has weird behaviour -> updated memory resulting with the value of 1 in the destination register (failure).
	 */

	/* Create an identity mapping of 1 MB on running kernel so that the kernel code can go ahead right after the MMU on */
	__pgtable[l1pte_index(CONFIG_RAM_BASE)] = CONFIG_RAM_BASE;
	set_l1_pte_sect_dcache(&__pgtable[l1pte_index(CONFIG_RAM_BASE)], L1_SECT_DCACHE_WRITEALLOC);

	/* Now, create a linear mapping in the kernel space */
#ifdef CONFIG_AVZ
	/* Map the hypervisor */
	for (i = 0; i < 12; i++) {
#else
	for (i = 0; i < 64; i++) {
#endif /* CONFIG_AVZ */
		__pgtable[l1pte_index(CONFIG_KERNEL_VADDR) + i] = CONFIG_RAM_BASE + i * TTB_SECT_SIZE;

		set_l1_pte_sect_dcache(&__pgtable[l1pte_index(CONFIG_KERNEL_VADDR) + i], L1_SECT_DCACHE_WRITEALLOC);
	}

#endif /* !CONFIG_SO3VIRT */

	/* At the moment, we keep a virtual mapping on the device tree - fdt_addr contains the physical address. */
	__pgtable[l1pte_index(fdt_addr)] = fdt_addr;
	set_l1_pte_sect_dcache(&__pgtable[l1pte_index(fdt_addr)], L1_SECT_DCACHE_WRITEALLOC);

	/* Early mapping I/O for UART */
	__pgtable[l1pte_index(CONFIG_UART_LL_PADDR)] = CONFIG_UART_LL_PADDR;
	set_l1_pte_sect_dcache(&__pgtable[l1pte_index(CONFIG_UART_LL_PADDR)], L1_SECT_DCACHE_OFF);

#ifndef CONFIG_SO3VIRT

#ifdef CONFIG_AVZ
	}
#endif /* CONFIG_AVZ */

	mmu_setup(__pgtable);

	dcache_enable();
	icache_enable();
#else

	mmu_page_table_flush((uint32_t) __pgtable, (uint32_t) (__pgtable + TTB_L1_ENTRIES));

#endif /* !CONFIG_SO3VIRT */

	/* Update the system page table using the virtual address */
	__sys_root_pgtable = (void *) (CONFIG_KERNEL_VADDR + TTB_L1_SYS_OFFSET);
}

/*
 * Clear the L1 PTE used for mapping of a specific virtual address.
 */
void clear_l1pte(void *l1pgtable, addr_t vaddr) {
	uint32_t *l1pte;

	/* If l1pgtable is NULL, we consider the system page table */
	if (l1pgtable == NULL)
		l1pgtable = __sys_root_pgtable;

	l1pte = l1pte_offset((uint32_t *) l1pgtable, vaddr);

	*l1pte = 0;

	flush_pte_entry(l1pte);
}

/* Duplicate the kernel area by doing a copy of L1 PTEs from the system page table */
void pgtable_copy_kernel_area(void *l1pgtable) {
	int i1;
	uint32_t *__l1pgtable = (uint32_t *) l1pgtable;

	for (i1 = l1pte_index(CONFIG_KERNEL_VADDR); i1 < TTB_L1_ENTRIES; i1++)
		__l1pgtable[i1] = ((uint32_t *) __sys_root_pgtable)[i1];

	mmu_page_table_flush((addr_t) __l1pgtable, (addr_t) (__l1pgtable + TTB_L1_ENTRIES));
}

/*
 * Allocate a new L1 page table. Return NULL if it fails.
 * The page table must be 16-KB aligned.
 */
void *new_root_pgtable(void) {
	void *pgtable;
#ifdef CONFIG_SO3VIRT
	int i;
#endif

	pgtable = memalign(4 * TTB_L1_ENTRIES, SZ_16K);
	if (!pgtable) {
		printk("%s: heap overflow...\n", __func__);
		kernel_panic();
	}

	/* Empty the page table */
	memset(pgtable, 0, 4 * TTB_L1_ENTRIES);

#ifdef CONFIG_SO3VIRT
	/* Let's copy 12 MB of hypervisor */
	for (i = 0; i < 12; i++)
		*l1pte_offset((u32 *) pgtable+i, avz_shared->hypervisor_vaddr) = *l1pte_offset((u32 *) __sys_root_pgtable+i, avz_shared->hypervisor_vaddr);
#endif

	return pgtable;
}

void copy_root_pgtable(void *dst, void *src) {
	memcpy(dst, src, TTB_L1_SIZE);
}

/**
 * Free a root page table and its associated L2 page tables used for the user space area.
 * We do not consider any shared pages/page tables.
 *
 * @param pgtable
 * @param remove  true if the root page table must be freed.
 */
void reset_root_pgtable(void *pgtable, bool remove) {
	int i;
	uint32_t *l1pte, *l2pte;

	for (i = 0; i < l1pte_index(CONFIG_KERNEL_VADDR); i++) {

		l1pte = (uint32_t *) pgtable + i;

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
		free(pgtable);

	mmu_page_table_flush((addr_t) pgtable, (addr_t) (pgtable + TTB_L1_ENTRIES));
}

void mmu_switch_kernel(void *pgtable_paddr) {

#ifdef CONFIG_SO3VIRT
	avz_shared->pagetable_paddr = (addr_t) pgtable_paddr;
#endif

	flush_dcache_all();

	/* Take care about the lower bits because
	 * it might be initialized by the Linux domain and should
	 * be preserved if any.
	 */

	if (((uint32_t) pgtable_paddr & TTBR0_BASE_ADDR_MASK) != (uint32_t) pgtable_paddr)
		WRITE_CP32((uint32_t) pgtable_paddr, TTBR0_32);
	else
		__mmu_switch_ttbr0(pgtable_paddr);

	invalidate_icache_all();
	__asm_invalidate_tlb_all();
}

/**
 * Switch memory context in the user space
 *
 * @param pgtable
 */
void mmu_switch(void *pgtable_paddr) {
	mmu_switch_kernel(pgtable_paddr);
}

/*
 * Duplicate the user space memory from a memory context to another.
 * The L1 and subsequent L2 page tables are duplicated accordingly.
 *
 * The user space has only small (4 KB) pages.
 *
 * @from is the process containing the L1 page table to be duplicated
 * @to is the process containing the (already allocated) L1 page table of the target memory context
 */

void duplicate_user_space(struct pcb *from, struct pcb *to) {
	int i, j;
	uint32_t l2pgtable_size;
	uint32_t *l1pte, *l2pte, *l2pgtable;
	uint32_t *l1pte_dst, *l2pte_dst,*l2pgtable_dst;
	addr_t paddr;
	void *vaddr;

	l2pgtable_size = TTB_L2_ENTRIES * sizeof(uint32_t);

	for (i = 0; i < l1pte_index(CONFIG_KERNEL_VADDR); i++) {
		l1pte = (uint32_t *) from->pgtable + i;

		if (*l1pte) {

			BUG_ON(l1pte_is_sect(*l1pte));

			l1pte_dst = (uint32_t *) to->pgtable + i;

			/* Allocate a new L2 page table for the copy */
			l2pgtable_dst = memalign(l2pgtable_size, SZ_1K);
			ASSERT(l2pgtable_dst != NULL);

			memset(l2pgtable_dst, 0, l2pgtable_size);

			*l1pte_dst = __pa((addr_t) l2pgtable_dst);

			set_l1_pte_page_dcache(l1pte_dst, L1_PAGE_DCACHE_WRITEALLOC);

			l2pgtable = (uint32_t *) __va(*l1pte & TTB_L1_PAGE_ADDR_MASK);

			for (j = 0; j < 256; j++) {
				l2pte = l2pgtable + j;
				if (*l2pte)  {

					l2pte_dst = l2pgtable_dst + j;

					/* Get a new free page */
					paddr = get_free_page();
					BUG_ON(!paddr);

					/* Add the new page to the process list */
					add_page_to_proc(to, (page_t *) phys_to_page(paddr));

					create_mapping(current_pgtable(), FIXMAP_MAPPING, paddr, PAGE_SIZE, false);

					*l2pte_dst = paddr;

					set_l2_pte_dcache(l2pte_dst, L2_DCACHE_WRITEALLOC);

					vaddr = (void *) pte_index_to_vaddr(i, j);

					memcpy((void *) FIXMAP_MAPPING, vaddr, PAGE_SIZE);

				}
			}
		}
	}
	release_mapping(current_pgtable(), FIXMAP_MAPPING, PAGE_SIZE);
}

void dump_pgtable(void *l1pgtable) {

	int i, j;
	uint32_t *l1pte, *l2pte;
	uint32_t *__l1pgtable = (uint32_t *) l1pgtable;

	lprintk("           ***** Page table dump *****\n");

	if (!l1pgtable)
		l1pgtable = current_pgtable();

	for (i = 0; i < TTB_L1_ENTRIES; i++) {
		l1pte = __l1pgtable + i;
#ifndef CONFIG_AVZ
		if (i < 0xff0) {
#endif
			if (*l1pte) {
				if (l1pte_is_sect(*l1pte))
					lprintk(" - L1 pte@%p (idx %x) mapping %x is section type  content: %x\n", __l1pgtable+i, i, i << (32 - TTB_L1_ORDER), *l1pte);
				else
					lprintk(" - L1 pte@%p (idx %x) is PT type   content: %x\n", __l1pgtable+i, i, *l1pte);

				if (!l1pte_is_sect(*l1pte)) {

					for (j = 0; j < 256; j++) {
						l2pte = ((uint32_t *) __va(*l1pte & TTB_L1_PAGE_ADDR_MASK)) + j;
						if (*l2pte)
							lprintk("      - L2 pte@%p (i2=%x) mapping %x  content: %x\n", l2pte, j, pte_index_to_vaddr(i, j), *l2pte);
					}
				}
			}
#ifndef CONFIG_AVZ
		}
#endif
	}
}

void dump_current_pgtable(void) {
	dump_pgtable(current_pgtable());
}

/*
 * Get the physical address from a virtual address (valid for any virt. address).
 * The function reads the page table(s).
 */
addr_t virt_to_phys_pt(addr_t vaddr) {
	uint32_t *l1pte, *l2pte;
	uint32_t offset;

	/* Get the L1 PTE. */
	l1pte = l1pte_offset(current_pgtable(), vaddr);
	BUG_ON(!*l1pte);

	offset = vaddr & ~PAGE_MASK;

	if (l1pte_is_sect(*l1pte)) {

		return (*l1pte & TTB_L1_SECT_ADDR_MASK) | offset;

	} else {

		l2pte = l2pte_offset(l1pte, vaddr);

		return (*l2pte & TTB_L2_ADDR_MASK) | offset;
	}

}
