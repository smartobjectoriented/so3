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

#ifndef MEMORY_H
#define MEMORY_H

#include <types.h>
#include <list.h>

#include <asm/memory.h>

#include <generated/autoconf.h>

/* Transitional page used for temporary mapping */
#define TRANSITIONAL_MAPPING	0xf0000000

#define IO_MAPPING_BASE		0xe0000000

extern struct list_head io_maplist;

/* Manage the io_maplist. The list is sorted by ascending vaddr. */
typedef struct {
	uint32_t vaddr;	/* Virtual address of the mapped I/O range */
	uint32_t paddr; /* Physical address of this mapping */
	size_t size;	/* Size in bytes */

	struct list_head list;
} io_map_t;

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE       (1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

struct mem_info {
    uint32_t phys_base;
    uint32_t size;
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

#define pfn_to_phys(pfn) ((pfn) << PAGE_SHIFT)
#define phys_to_pfn(phys) (((uint32_t) phys) >> PAGE_SHIFT)
#define virt_to_pfn(virt) (phys_to_pfn(__va((uint32_t) virt)))

#define __pa(vaddr) (((uint32_t) vaddr) - CONFIG_KERNEL_VIRT_ADDR + ((uint32_t) CONFIG_RAM_BASE))
#define __va(paddr) (((uint32_t) paddr) - ((uint32_t) CONFIG_RAM_BASE) + CONFIG_KERNEL_VIRT_ADDR)

#define page_to_pfn(page) ((uint32_t) ((uint32_t) ((page)-frame_table) + phys_to_pfn(__pa((uint32_t) frame_table))))
#define pfn_to_page(pfn) (&frame_table[((pfn)-(__pa((uint32_t) frame_table) >> PAGE_SHIFT))])

#define page_to_phys(page) (pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys) (pfn_to_page(phys_to_pfn(phys)))

void clear_bss(void);
void init_mmu(void);
void memory_init(void);

void frame_table_init(uint32_t frame_table_start);

/* Get memory informations from a device tree */
int get_mem_info(const void *fdt, mem_info_t *info);

void dump_frame_table(void);

uint32_t get_free_page(void);
void free_page(uint32_t paddr);

uint32_t get_free_vpage(void);
void free_vpage(uint32_t vaddr);

uint32_t get_contig_free_pages(uint32_t nrpages);
uint32_t get_contig_free_vpages(uint32_t nrpages);
void free_contig_pages(uint32_t page_phys, uint32_t nrpages);
void free_contig_vpages(uint32_t page_phys, uint32_t nrpages);

uint32_t get_kernel_size(void);

uint32_t *current_pgtable(void);

extern uint32_t *__current_pgtable;

static inline void set_pgtable(uint32_t *pgtable) {
	__current_pgtable = pgtable;
}

void init_io_mapping(void);
uint32_t io_map(uint32_t phys, size_t size);
void io_unmap(uint32_t vaddr);
io_map_t *find_io_map_by_paddr(uint32_t paddr);
void readjust_io_map(unsigned pfn_offset);

void dump_io_maplist(void);

#endif /* MEMORY_H */
