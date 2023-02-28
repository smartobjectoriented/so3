/*
 * Copyright (C) 2015-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef MMU_H
#define MMU_H

#ifndef __ASSEMBLY__
#include <types.h>
#endif

#include <sizes.h>

#ifdef CONFIG_AVZ
#define AGENCY_VOFFSET	UL(0xc0000000)
#define ME_VOFFSET	UL(0xc0000000)

#define L_TEXT_OFFSET	0x8000

#endif

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE       (1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

#define USER_SPACE_VADDR	0x1000ul

/* Memory space all I/O mapped registers and additional mappings */
#define IO_MAPPING_BASE		0xe0000000

/* Ramdev rootfs if any at this location */
#define RAMDEV_VADDR		0xd0000000

#define TTB_L1_SYS_OFFSET	0x4000

/* Fixmap page used for temporary mapping */
#define FIXMAP_MAPPING		0xf0000000

/* Define the number of entries in each page table */

#define TTB_L1_ORDER      12
#define TTB_L2_ORDER      8

#define TTB_I1_SHIFT	  (32 - TTB_L1_ORDER)
#define TTB_I1_MASK	  (~((1 << TTB_I1_SHIFT)-1))

#define TTB_I2_SHIFT	  PAGE_SHIFT
#define TTB_I2_MASK	  (~((TTB_I2_SHIFT)-1))

#define TTB_L1_ENTRIES    (1 << TTB_L1_ORDER)
#define TTB_L2_ENTRIES    (1 << TTB_L2_ORDER)

/* Size of the L1 page table */
#define TTB_L1_SIZE    	  (4 << TTB_L1_ORDER)

/* To get the address of the L2 page table from a L1 descriptor */
#define TTB_L1_SECT_ADDR_SHIFT	20
#define TTB_L1_SECT_ADDR_OFFSET	(1 << TTB_L1_SECT_ADDR_SHIFT)
#define TTB_L1_SECT_ADDR_MASK	(~(TTB_L1_SECT_ADDR_OFFSET - 1))

#define TTB_L1_PAGE_ADDR_SHIFT	10
#define TTB_L1_PAGE_ADDR_OFFSET	(1 << TTB_L1_PAGE_ADDR_SHIFT)
#define TTB_L1_PAGE_ADDR_MASK	(~(TTB_L1_PAGE_ADDR_OFFSET - 1))

#define TTB_SECT_SIZE	(0x100000)
#define TTB_SECT_MASK   (~(TTB_SECT_SIZE - 1))

#define TTB_L2_ADDR_MASK	(~(PAGE_SIZE-1))

/* Given a virtual address, get an entry offset into a page table. */
#define l1pte_index(a) (((uint32_t) a) >> (32 - TTB_L1_ORDER))
#define l2pte_index(a) ((((uint32_t) a) >> PAGE_SHIFT) & (TTB_L2_ENTRIES - 1))

#define pte_index_to_vaddr(i1, i2) ((i1 << TTB_I1_SHIFT) | (i2 << TTB_I2_SHIFT))

#define l1pte_offset(pgtable, addr)     ((uint32_t *) pgtable + l1pte_index(addr))
#define l2pte_offset(l1pte, addr) 	((uint32_t *) __va(*l1pte & TTB_L1_PAGE_ADDR_MASK) + l2pte_index(addr))
#define l2pte_first(l1pte)		((uint32_t *) __va(*l1pte & TTB_L1_PAGE_ADDR_MASK))

#define l1sect_addr_end(addr, end)                                         \
 ({      unsigned long __boundary = ((addr) + TTB_SECT_SIZE) & TTB_SECT_MASK;  \
         (__boundary - 1 < (end) - 1) ? __boundary : (end);                \
 })

/* Short-Descriptor Translation Table Level 1 Bits */

#define TTB_L1_RES0		(1 << 4)
#define TTB_L1_IMPDEF		(1 << 9)
#define TTB_DOMAIN(x)		((x & 0xf) << 5)

/* Page Table related bits */
#define TTB_L1_PAGE_PXN		(1 << 2)
#define TTB_L1_PAGE_NS		(1 << 3)
#define TTB_L1_PAGE		(1 << 0)

/* Section related bits */
#define TTB_L1_NG		(1 << 17)
#define TTB_L1_S		(1 << 16)
#define TTB_L1_SECT_NS		(1 << 19)
#define TTB_L1_TEX(x)		((x & 0x7) << 12)
#define TTB_L1_SECT_PXN		(1 << 0)
#define TTB_L1_XN		(1 << 4)
#define TTB_L1_C		(1 << 3)
#define TTB_L1_B		(1 << 2)
#define TTB_L1_SECT		(2 << 0)
#define TTB_L1_L2		(1 << 0)

/* R/W in kernel and user mode */
#define TTB_L1_AP		(3 << 10)

/* Short-Descriptor Translation Table Level 2 Bits */

#define TTB_L2_NG		(1 << 11)
#define TTB_L2_S		(1 << 10)

#define TTB_L2_TEX(x)		((x & 0x7) << 6)
#define TTB_L2_XN		(1 << 0)
#define TTB_L2_C		(1 << 3)
#define TTB_L2_B		(1 << 2)
#define TTB_L2_PAGE		(2 << 0)

/* R/W in kernel and user mode */
#define TTB_L2_AP		(3 << 4)

 /* TTBR0 bits */
 #define TTBR0_BASE_ADDR_MASK	0xFFFFC000
 #define TTBR0_RGN_NC		(0 << 3)
 #define TTBR0_RGN_WBWA		(1 << 3)
 #define TTBR0_RGN_WT		(2 << 3)
 #define TTBR0_RGN_WB		(3 << 3)

 /* TTBR0[6] is IRGN[0] and TTBR[0] is IRGN[1] */
 #define TTBR0_IRGN_NC		(0 << 0 | 0 << 6)
 #define TTBR0_IRGN_WBWA	(0 << 0 | 1 << 6)
 #define TTBR0_IRGN_WT		(1 << 0 | 0 << 6)
 #define TTBR0_IRGN_WB		(1 << 0 | 1 << 6)


#ifndef __ASSEMBLY__

#include <common.h>
#include <types.h>

#ifdef CONFIG_MMU

static inline bool l1pte_is_sect(uint32_t l1pte) {

	/* Check if the L1 pte is for mapping of section or not */
	return (l1pte & TTB_L1_SECT);
}

static inline bool l1pte_is_pt(uint32_t l1pte) {

	/* Check if the L1 pte is for mapping of section or not */
	return ((l1pte & TTB_L1_L2) && !(l1pte & TTB_L1_SECT));
}

/* Options available for data cache related to section */
enum ttb_l1_sect_dcache_option {
	L1_SECT_DCACHE_OFF = TTB_DOMAIN(0) | TTB_L1_SECT,
	L1_SECT_DCACHE_WRITETHROUGH = L1_SECT_DCACHE_OFF | TTB_L1_C,
	L1_SECT_DCACHE_WRITEBACK = L1_SECT_DCACHE_WRITETHROUGH | TTB_L1_B,
	L1_SECT_DCACHE_WRITEALLOC = L1_SECT_DCACHE_WRITEBACK | TTB_L1_TEX(1),
};

enum ttb_l1_page_dcache_option {
	L1_PAGE_DCACHE_OFF = TTB_DOMAIN(0) | TTB_L1_PAGE | TTB_L1_RES0,
	L1_PAGE_DCACHE_WRITETHROUGH = L1_PAGE_DCACHE_OFF,
	L1_PAGE_DCACHE_WRITEBACK = L1_PAGE_DCACHE_WRITETHROUGH,
	L1_PAGE_DCACHE_WRITEALLOC = L1_PAGE_DCACHE_WRITEBACK,
};

/* Options available for data cache related to a page */
enum ttb_l2_dcache_option {
	L2_DCACHE_OFF = TTB_L2_PAGE,
	L2_DCACHE_WRITETHROUGH = L2_DCACHE_OFF | TTB_L2_C,
	L2_DCACHE_WRITEBACK = L2_DCACHE_WRITETHROUGH | TTB_L2_B,
	L2_DCACHE_WRITEALLOC = L2_DCACHE_WRITEBACK | TTB_L2_TEX(1),
};

#include <process.h>

extern void *__sys_root_pgtable;

void *current_pgtable(void);

extern void *__current_pgtable;

static inline void set_pgtable(addr_t *pgtable) {
	__current_pgtable = pgtable;
}

extern void __mmu_switch_ttbr0(void *root_pgtable_phys);

void mmu_switch_kernel(void *pgtable_paddr);

void *current_pgtable(void);
void *new_root_pgtable(void);
void copy_root_pgtable(void *dst, void *src);

addr_t virt_to_phys_pt(addr_t vaddr);

void pgtable_copy_kernel_area(void *l1pgtable);

void create_mapping(void *l1pgtable, addr_t virt_base, addr_t phys_base, uint32_t size, bool nocache);
void release_mapping(void *pgtable, addr_t virt_base, uint32_t size);

void reset_root_pgtable(void *pgtable, bool remove);
void dump_pgtable(void *l1pgtable);

void mmu_get_current_pgtable(addr_t *pgtable_paddr);

void clear_l1pte(void *l1pgtable, addr_t vaddr);

void mmu_switch(void *l1pgtable);

void ramdev_create_mapping(void *root_pgtable, addr_t ramdev_start, addr_t ramdev_end);

void flush_tlb_all(void);

void dump_current_pgtable(void);

void set_l1_pte_sect_dcache(uint32_t *l1pte, enum ttb_l1_sect_dcache_option option);
void set_l1_pte_page_dcache(uint32_t *l1pte, enum ttb_l1_page_dcache_option option);
void set_l2_pte_dcache(uint32_t *l2pte, enum ttb_l2_dcache_option option);

void mmu_setup(void *pgtable);

#endif /* CONFIG_MMU */

#endif


#endif /* MMU_H */

