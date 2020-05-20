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

#include <common.h>
#include <types.h>
#include <memory.h>
#include <spinlock.h>
#include <sizes.h>
#include <process.h>
#include <heap.h>

#include <device/ramdev.h>

#include <asm/mmu.h>
#include <asm/cacheflush.h>

uint32_t *__sys_l1pgtable;

page_t *frame_table;
static spinlock_t ft_lock;
static uint32_t ft_pfn_end;

/* Page-aligned kernel size (including frame table) */
static uint32_t kernel_size;

/* Current available I/O range address */
uint32_t io_mapping_current;
struct list_head io_maplist;

uint32_t *__current_pgtable;

uint32_t *current_pgtable(void) {
	return __current_pgtable;
}

/* Initialize the frame table */
void frame_table_init(uint32_t frame_table_start) {
	uint32_t i, ft_phys, ft_length, ft_pages;
	uint32_t ramdev_size = 0;

	/* The frame table (ft) is placed (page-aligned) right after the kernel region. */
	ft_phys = ALIGN_UP(__pa(frame_table_start), PAGE_SIZE);

	frame_table = (page_t *) __va(ft_phys);

	/* Size of the available memory (without the kernel region) */
#ifdef CONFIG_RAMDEV
	ramdev_size = get_ramdev_size();
#endif
	mem_info.avail_pages = (mem_info.size - (ft_phys - mem_info.phys_base) - ramdev_size) >> PAGE_SHIFT;

	/* Determine the length of the frame table in bytes */
	ft_length = mem_info.avail_pages * sizeof(page_t);

	/* Keep the frame table with a page size granularity */
	ft_pages = (ALIGN_UP(ft_length, PAGE_SIZE)) >> PAGE_SHIFT;

	for (i = 0; i < ft_pages; i++)
		frame_table[i].free = false;

	for (i = ft_pages; i < mem_info.avail_pages; i++)
		frame_table[i].free = true;

	ft_pfn_end = (ft_phys >> PAGE_SHIFT) + ft_pages - 1;

	/* Set the definitive kernel size */
	kernel_size = (ft_pfn_end << PAGE_SHIFT) - CONFIG_RAM_BASE + 1;

	printk("SO3 Memory information:\n");

	printk("  - kernel size including frame table is: %d (%x) bytes, %d MB\n", kernel_size, kernel_size, kernel_size / SZ_1M);
	printk("  - Frame table size is: %d bytes\n", ft_length);

	spin_lock_init(&ft_lock);
}

uint32_t get_kernel_size(void) {
	return kernel_size;
}

/*
 * Get a free page. Return the physical address of the page (or 0 if not available).
 */
uint32_t get_free_page(void) {
	uint32_t i;

	spin_lock(&ft_lock);
	for (i = 0; i < mem_info.avail_pages; i++) {
		if (frame_table[i].free) {
			frame_table[i].free = false;

			spin_unlock(&ft_lock);

			/* Found an available page */
			return page_to_phys(&frame_table[i]);
		}
	}
	spin_unlock(&ft_lock);

	/* No available page */
	return 0;
}

/*
 * Get a free page with a virtual mapping.
 */
uint32_t get_free_vpage(void) {
	uint32_t paddr, vaddr;

	paddr = get_free_page();
	ASSERT(paddr);

	vaddr = io_map(paddr, PAGE_SIZE);

	return vaddr;
}

/*
 * Release a page, mark as free.
 */
void free_page(uint32_t paddr) {
	page_t *page;

	spin_lock(&ft_lock);

	page = phys_to_page(paddr);
	page->free = true;

	spin_unlock(&ft_lock);
}

void free_vpage(uint32_t vaddr) {
	uint32_t paddr;

	paddr = virt_to_phys_pt(vaddr);

	io_unmap(vaddr);
	free_page(paddr);
}



/*
 * Search for a number of contiguous pages.
 * Returns 0 if not available.
 */
uint32_t get_contig_free_pages(uint32_t nrpages) {
	uint32_t i, base = 0;

	spin_lock(&ft_lock);

	for (i = 0; i < mem_info.avail_pages; i++) {
		if (frame_table[i].free) {
			if (i-base+1 == nrpages) {
				/* Set the page as busy */
				for (i = 0; i < nrpages; i++)
					frame_table[base+i].free = false;

				spin_unlock(&ft_lock);

				/* Returns the block base */
				return page_to_phys(&frame_table[base]);
			}
		} else
			base = i+1;
	}

	spin_unlock(&ft_lock);

	return 0;
}

/*
 * Search for a number of contiguous physical and virtual pages.
 * Returns 0 if not available.
 */
uint32_t get_contig_free_vpages(uint32_t nrpages) {
	uint32_t vaddr, paddr;

	paddr = get_contig_free_pages(nrpages);
	ASSERT(paddr);

	vaddr = io_map(paddr, nrpages * PAGE_SIZE);

	return vaddr;
}


void free_contig_pages(uint32_t paddr, uint32_t nrpages) {
	uint32_t i;
	page_t *page;

	spin_lock(&ft_lock);

	page = phys_to_page(paddr);

	for (i = 0; i < nrpages; i++)
		(page + i)->free = true;

	spin_unlock(&ft_lock);

}

void free_contig_vpages(uint32_t vaddr, uint32_t nrpages) {
	uint32_t paddr;

	paddr = virt_to_phys_pt(vaddr);

	io_unmap(vaddr);
	free_contig_pages(paddr, nrpages);
}

void dump_frame_table(void) {
	int i;

	printk("** Dump of frame table contents **\n\n");

	for (i = 0; i < mem_info.avail_pages; i++)
		printk("  - Page address (phys) :%x, free: %d\n", virt_to_phys_pt((uint32_t) &frame_table[i]), frame_table[i].free);
}

/*
 * I/O address space management
 */

/* Init the I/O address space */
void init_io_mapping(void) {
	io_mapping_current = IO_MAPPING_BASE;
}

void dump_io_maplist(void) {
	io_map_t *cur = NULL;
	struct list_head *pos;

	printk("%s: ***** List of I/O mappings *****\n\n", __func__);

	list_for_each(pos, &io_maplist) {

		cur = list_entry(pos, io_map_t, list);

		printk("    - vaddr: %x  mapped on   paddr: %x\n", cur->vaddr, cur->paddr);
		printk("          with size: %d bytes\n", cur->size);
	}
}

/* Map a I/O address range to its physical range */
uint32_t io_map(uint32_t phys, size_t size) {
	io_map_t *io_map;
	struct list_head *pos;
	io_map_t *cur = NULL;
	uint32_t target, offset;

	/* Sometimes, it may happen than drivers try to map several devices which are located within the same page,
	 * i.e. the 4-KB page offset is not null.
	 */
	offset = phys & (PAGE_SIZE -1);
	phys = phys & PAGE_MASK;

	io_map = find_io_map_by_paddr(phys);
	if (io_map) {
		if (io_map->size == size)
			return io_map->vaddr + offset;
		else
			BUG();
	}

	/* We are looking for a free region in the virtual address space */
	target = IO_MAPPING_BASE;

	/* Re-adjust the address according to the alignment */

	list_for_each(pos, &io_maplist) {
		cur = list_entry(pos, io_map_t, list);
		if (target + size <= cur->vaddr)
			break;
		else {
			target = cur->vaddr + cur->size;
			target = ALIGN_UP(target, ((size < SZ_1M) ? PAGE_SIZE : SZ_1M));

			/* If we reach the end of the list, we can detect it. */
			cur = NULL;
		}
	}

	io_map = (io_map_t *) malloc(sizeof(io_map_t));
	ASSERT(io_map != NULL);

	io_map->vaddr = target;
	io_map->paddr = phys;
	io_map->size = size;

	/* Insert the new entry before <cur> or if NULL at the tail of the list. */
	if (cur != NULL) {

		io_map->list.prev = pos->prev;
		io_map->list.next = pos;

		pos->prev->next = &io_map->list;
		pos->prev = &io_map->list;

	} else
		list_add_tail(&io_map->list, &io_maplist);


	create_mapping(NULL, io_map->vaddr, io_map->paddr, io_map->size, true);

	flush_tlb_all();
	cache_clean_flush();

	return io_map->vaddr + offset;

}

/*
 * Try to find an io_map entry corresponding to a specific pvaddr .
 */
io_map_t *find_io_map_by_paddr(uint32_t paddr) {
	struct list_head *pos;
	io_map_t *io_map;

	list_for_each(pos, &io_maplist) {
		io_map = list_entry(pos, io_map_t, list);
		if (io_map->paddr == paddr)
			return io_map;
	}

	return NULL;
}

/*
 * Remove a mapping.
 */
void io_unmap(uint32_t vaddr) {
	io_map_t *cur = NULL;
	struct list_head *pos, *q;

	/* If we have an 4 KB offset, we do not have the mapping at this level. */
	vaddr = vaddr & PAGE_MASK;

	list_for_each_safe(pos, q, &io_maplist) {

		cur = list_entry(pos, io_map_t, list);

		if (cur->vaddr == vaddr) {
			list_del(pos);
			break;
		}
		cur = NULL;
	}

	if (cur == NULL) {
		lprintk("io_unmap failure: did not find entry for vaddr %x\n", vaddr);
		kernel_panic();
	}

	release_mapping(NULL, cur->vaddr, cur->size);

	free(cur);
}

void readjust_io_map(unsigned pfn_offset) {
	io_map_t *io_map;
	struct list_head *pos;

	list_for_each(pos, &io_maplist) {
		io_map = list_entry(pos, io_map_t, list);
		io_map->paddr += pfn_to_phys(pfn_offset);
	}


}
