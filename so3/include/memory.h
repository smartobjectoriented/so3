/*
 * Copyright (C) 2020 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef MEMORY_H
#define MEMORY_H

#ifndef __ASSEMBLY__

#include <common.h>

#include <types.h>
#include <list.h>
#include <bitops.h>

#ifdef CONFIG_AVZ
#include <avz/memslot.h>
#endif

#endif /* __ASSEMBLY__ */

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE       (1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

#ifndef __ASSEMBLY__

#ifdef CONFIG_SO3VIRT

extern addr_t avz_guest_phys_offset;

#undef CONFIG_RAM_BASE
#define CONFIG_RAM_BASE	avz_guest_phys_offset

#endif

extern struct list_head io_maplist;

/* Manage the io_maplist. The list is sorted by ascending vaddr. */
typedef struct {
	addr_t vaddr;	/* Virtual address of the mapped I/O range */
	addr_t paddr; /* Physical address of this mapping */
	size_t size;	/* Size in bytes */

	struct list_head list;
} io_map_t;

struct mem_info {
	addr_t phys_base;
	unsigned long size;
	uint32_t avail_pages; /* Available pages including frame table, without the low kernel region */
};
typedef struct mem_info mem_info_t;

extern mem_info_t mem_info;

/*
 * Frame table which is constituted by the set of struct page.
 */
struct page {
	/* If the page is not mapped yet, and hence free. */
	bool free;

	/* Number of reference to this page. If the process is fork'd,
	 * the child will also have reference to the page.
	 */
	uint32_t refcount;

};
typedef struct page page_t;

extern page_t *frame_table;
extern volatile addr_t pfn_start;

#ifdef CONFIG_AVZ

/*
 * We add two functions for retrieving virt and phys address relative to
 * Linux offset according to the memory map (used to access guest mem)
 */
#define __lpa(vaddr) ((vaddr) - AGENCY_VOFFSET + memslot[MEMSLOT_AGENCY].base_paddr)
#define __lva(paddr) ((paddr) - memslot[MEMSLOT_AGENCY].base_paddr + AGENCY_VOFFSET)

void put_ME_slot(unsigned int ME_slotID);
int get_ME_free_slot(unsigned int size, ME_state_t ME_state);

#endif /* CONFIG_AVZ */

#define pfn_to_phys(pfn) ((pfn) << PAGE_SHIFT)
#define phys_to_pfn(phys) (((addr_t) phys) >> PAGE_SHIFT)
#define virt_to_pfn(virt) (phys_to_pfn(__va((addr_t) virt)))

#define ipa_offset(memslot)		(memslot.ipa_addr - memslot.base_paddr)

#define virt_to_ipa(memslot, va)	(phys_to_ipa(memslot, __pa(va)))
#define phys_to_ipa(memslot, pa) 	(((addr_t) pa) + ipa_offset(memslot))

#define ipa_to_phys(memslot, ipa)	(((addr_t) ipa) - ipa_offset(memslot))

#define ipa_to_lva(memslot, ipa)	(__lva(ipa_to_phys(memslot, ipa)))

#define __pa(vaddr) (((addr_t) vaddr) - CONFIG_KERNEL_VADDR + ((addr_t) CONFIG_RAM_BASE))
#define __va(paddr) (((addr_t) paddr) - ((addr_t) CONFIG_RAM_BASE) + CONFIG_KERNEL_VADDR)

#define page_to_pfn(page) ((addr_t) ((addr_t) (page - frame_table) + pfn_start))
#define pfn_to_page(pfn) (&frame_table[pfn - pfn_start])

#define page_to_phys(page) (pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys) (pfn_to_page(phys_to_pfn(phys)))

void clear_bss(void);
void init_mmu(void);
void memory_init(void);

void frame_table_init(addr_t frame_table_start);

struct domain;
struct page_info;
struct vcpu;

/* Get memory informations from a device tree */
int get_mem_info(const void *fdt, mem_info_t *info);

void dump_frame_table(void);

addr_t get_free_page(void);
void free_page(addr_t paddr);

addr_t get_free_vpage(void);
void free_vpage(addr_t vaddr);

addr_t get_contig_free_pages(uint32_t nrpages);
addr_t get_contig_free_vpages(uint32_t nrpages);
void free_contig_pages(addr_t page_phys, uint32_t nrpages);
void free_contig_vpages(addr_t page_phys, uint32_t nrpages);
extern page_t *frame_table;
extern volatile addr_t pfn_start;

void clear_bss(void);
void init_mmu(void);
void memory_init(void);

void frame_table_init(addr_t frame_table_start);
extern int __irq_safe[];

extern struct domain *idle_domain[];

void early_memory_init(void);
void memory_init(void);

uint32_t get_kernel_size(void);

struct pcb;
void duplicate_user_space(struct pcb *from, struct pcb *to);

void init_io_mapping(void);
addr_t io_map(addr_t phys, size_t size);
void io_unmap(addr_t vaddr);
io_map_t *find_io_map_by_paddr(addr_t paddr);

void dump_io_maplist(void);

void switch_mm_domain(struct domain *d);

void dump_page(unsigned int pfn);

static inline int get_order_from_bytes(addr_t size)
{
	int order;

	size = (size - 1) >> PAGE_SHIFT;
	for (order = 0; size; order++)
		size >>= 1;

	return order;
}

void clear_bss(void);

#ifdef CONFIG_SOO
void readjust_io_map(long pfn_offset);
#endif /* CONFIG_SOO */

#endif /* __ASSEMBLY__ */

#endif /* MEMORY_H */
