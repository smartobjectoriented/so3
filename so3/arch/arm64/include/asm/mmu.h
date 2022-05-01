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

#define USER_SPACE_VADDR		UL(0x1000)

#define IO_MAPPING_BASE			UL(0xffff900000000000)

/* The user space can be up to bits [47:0] and uses ttbr0_el1
 * as main L0 page table.
 */

#define USER_STACK_TOP_VADDR		UL(0x0001000000000000)

#define	SZ_256G		(256UL * SZ_1G)

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE       (1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

#define PAGE_OFFSET	UL(0xffff800000080000)

/* Order of size which makes sense in block mapping */
#define BLOCK_256G_OFFSET	(SZ_256G - 1)
#define BLOCK_1G_OFFSET		(SZ_1G - 1)
#define BLOCK_2M_OFFSET		(SZ_2M - 1)

#define BLOCK_256G_MASK		(~BLOCK_256G_OFFSET)
#define BLOCK_1G_MASK		(~BLOCK_1G_OFFSET)
#define BLOCK_2M_MASK		(~BLOCK_2M_OFFSET)

/*
 *  With 4k page granule, a virtual address is split into 4 lookup parts
 *  spanning 9 bits each:
 *
 *    _______________________________________________
 *   |       |       |       |       |       |       |
 *   |   0   |  L0   |  L1   |  L2   |  L3   |  off  |
 *   |_______|_______|_______|_______|_______|_______|
 *     63-48   47-39   38-30   29-21   20-12   11-00
 *
 *             mask        block size
 *
 *    L0: FF8000000000       512GB
 *    L1:   7FC0000000       1G
 *    L2:     3FE00000       2M
 *    L3:       1FF000       4K
 *    off:         FFF
 */

/* Define the number of entries in each page table */

#define TTB_L0_ORDER      9
#define TTB_L1_ORDER      9
#define TTB_L2_ORDER      9
#define TTB_L3_ORDER      9

#define TTB_I0_SHIFT	  39
#define TTB_I0_MASK	  (~((1UL << TTB_I0_SHIFT)-1))

#define TTB_I1_SHIFT	  30
#define TTB_I1_MASK	  (~((1UL << TTB_I1_SHIFT)-1))

#define TTB_I2_SHIFT	  21
#define TTB_I2_MASK	  (~((1UL << TTB_I2_SHIFT)-1))

#define TTB_I3_SHIFT	  12
#define TTB_I3_MASK	  (~((1UL << TTB_I3_SHIFT)-1))

#define TTB_L0_ENTRIES    (1UL << TTB_L0_ORDER)
#define TTB_L1_ENTRIES    (1UL << TTB_L1_ORDER)
#define TTB_L2_ENTRIES    (1UL << TTB_L2_ORDER)
#define TTB_L3_ENTRIES    (1UL << TTB_L3_ORDER)

/* Size of the page tables */
#define TTB_L0_SIZE    	  (8UL << TTB_L0_ORDER)
#define TTB_L1_SIZE    	  (8UL << TTB_L1_ORDER)
#define TTB_L2_SIZE    	  (8UL << TTB_L2_ORDER)
#define TTB_L3_SIZE    	  (8UL << TTB_L3_ORDER)

/*
 * Memory types available.
 */
#define MT_NORMAL		0
#define MT_NORMAL_TAGGED	1
#define MT_NORMAL_NC		2
#define MT_NORMAL_WT		3
#define MT_DEVICE_nGnRnE	4
#define MT_DEVICE_nGnRE		5
#define MT_DEVICE_GRE		6

/* MAIR_ELx memory attributes (used by Linux) */
#define MAIR_ATTR_DEVICE_nGnRnE		UL(0x00)
#define MAIR_ATTR_DEVICE_nGnRE		UL(0x04)
#define MAIR_ATTR_DEVICE_GRE		UL(0x0c)
#define MAIR_ATTR_NORMAL_NC		UL(0x44)
#define MAIR_ATTR_NORMAL_WT		UL(0xbb)
#define MAIR_ATTR_NORMAL_TAGGED		UL(0xf0)
#define MAIR_ATTR_NORMAL		UL(0xff)
#define MAIR_ATTR_MASK			UL(0xff)

/* Position the attr at the correct index */
#define MAIR_ATTRIDX(attr, idx)		((attr) << ((idx) * 8))

#define MAIR_EL1_SET							\
	(MAIR_ATTRIDX(MAIR_ATTR_DEVICE_nGnRnE, MT_DEVICE_nGnRnE) |	\
	 MAIR_ATTRIDX(MAIR_ATTR_DEVICE_nGnRE, MT_DEVICE_nGnRE) |	\
	 MAIR_ATTRIDX(MAIR_ATTR_DEVICE_GRE, MT_DEVICE_GRE) |		\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL_NC, MT_NORMAL_NC) |		\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL, MT_NORMAL) |			\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL_WT, MT_NORMAL_WT) |		\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL, MT_NORMAL_TAGGED))

/*
 * Hardware page table definitions.
 *
 */

#define PTE_TYPE_MASK		(3 << 0)
#define PTE_TYPE_FAULT		(0 << 0)
#define PTE_TYPE_TABLE		(3 << 0)
#define PTE_TYPE_PAGE		(3 << 0)
#define PTE_TYPE_BLOCK		(1 << 0)
#define PTE_TYPE_VALID		(1 << 0)

#define PTE_TABLE_PXN		(1UL << 59)
#define PTE_TABLE_XN		(1UL << 60)
#define PTE_TABLE_AP		(1UL << 61)
#define PTE_TABLE_NS		(1UL << 63)

/*
 * Block
 */
#define PTE_BLOCK_MEMTYPE(x)	((x) << 2)
#define PTE_BLOCK_NS            (1UL << 5)
#define PTE_BLOCK_AP1		(1UL << 6)
#define PTE_BLOCK_AP2		(1UL << 7)
#define PTE_BLOCK_NON_SHARE	(0UL << 8)
#define PTE_BLOCK_OUTER_SHARE	(2UL << 8)
#define PTE_BLOCK_INNER_SHARE	(3UL << 8)
#define PTE_BLOCK_AF		(1UL << 10)
#define PTE_BLOCK_NG		(1UL << 11)
#define PTE_BLOCK_DBM		(1UL << 51)
#define PTE_BLOCK_PXN		(1UL << 53)
#define PTE_BLOCK_UXN		(1UL << 54)

/*
 * TCR flags.
 */

#define TCR_T0SZ_OFFSET		0
#define TCR_T1SZ_OFFSET		16
#define TCR_T0SZ(x)		((UL(64) - (x)) << TCR_T0SZ_OFFSET)
#define TCR_T1SZ(x)		((UL(64) - (x)) << TCR_T1SZ_OFFSET)
#define TCR_TxSZ(x)		(TCR_T0SZ(x) | TCR_T1SZ(x))
#define TCR_TxSZ_WIDTH		6
#define TCR_T0SZ_MASK		(((UL(1) << TCR_TxSZ_WIDTH) - 1) << TCR_T0SZ_OFFSET)

#define TCR_EPD0_SHIFT		7
#define TCR_EPD0_MASK		(UL(1) << TCR_EPD0_SHIFT)
#define TCR_IRGN0_SHIFT		8
#define TCR_IRGN0_MASK		(UL(3) << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_NC		(UL(0) << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WBWA		(UL(1) << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WT		(UL(2) << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WBnWA		(UL(3) << TCR_IRGN0_SHIFT)

#define TCR_EPD1_SHIFT		23
#define TCR_EPD1_MASK		(UL(1) << TCR_EPD1_SHIFT)
#define TCR_IRGN1_SHIFT		24
#define TCR_IRGN1_MASK		(UL(3) << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_NC		(UL(0) << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WBWA		(UL(1) << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WT		(UL(2) << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WBnWA		(UL(3) << TCR_IRGN1_SHIFT)

#define TCR_IRGN_NC		(TCR_IRGN0_NC | TCR_IRGN1_NC)
#define TCR_IRGN_WBWA		(TCR_IRGN0_WBWA | TCR_IRGN1_WBWA)
#define TCR_IRGN_WT		(TCR_IRGN0_WT | TCR_IRGN1_WT)
#define TCR_IRGN_WBnWA		(TCR_IRGN0_WBnWA | TCR_IRGN1_WBnWA)
#define TCR_IRGN_MASK		(TCR_IRGN0_MASK | TCR_IRGN1_MASK)

#define TCR_ORGN0_SHIFT		10
#define TCR_ORGN0_MASK		(UL(3) << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_NC		(UL(0) << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WBWA		(UL(1) << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WT		(UL(2) << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WBnWA		(UL(3) << TCR_ORGN0_SHIFT)

#define TCR_ORGN1_SHIFT		26
#define TCR_ORGN1_MASK		(UL(3) << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_NC		(UL(0) << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WBWA		(UL(1) << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WT		(UL(2) << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WBnWA		(UL(3) << TCR_ORGN1_SHIFT)

#define TCR_ORGN_NC		(TCR_ORGN0_NC | TCR_ORGN1_NC)
#define TCR_ORGN_WBWA		(TCR_ORGN0_WBWA | TCR_ORGN1_WBWA)
#define TCR_ORGN_WT		(TCR_ORGN0_WT | TCR_ORGN1_WT)
#define TCR_ORGN_WBnWA		(TCR_ORGN0_WBnWA | TCR_ORGN1_WBnWA)
#define TCR_ORGN_MASK		(TCR_ORGN0_MASK | TCR_ORGN1_MASK)

#define TCR_SH0_SHIFT		12
#define TCR_SH0_MASK		(UL(3) << TCR_SH0_SHIFT)
#define TCR_SH0_INNER		(UL(3) << TCR_SH0_SHIFT)

#define TCR_SH1_SHIFT		28
#define TCR_SH1_MASK		(UL(3) << TCR_SH1_SHIFT)
#define TCR_SH1_INNER		(UL(3) << TCR_SH1_SHIFT)
#define TCR_SHARED		(TCR_SH0_INNER | TCR_SH1_INNER)

#define TCR_TG0_SHIFT		14
#define TCR_TG0_MASK		(UL(3) << TCR_TG0_SHIFT)
#define TCR_TG0_4K		(UL(0) << TCR_TG0_SHIFT)
#define TCR_TG0_64K		(UL(1) << TCR_TG0_SHIFT)
#define TCR_TG0_16K		(UL(2) << TCR_TG0_SHIFT)

#define TCR_TG1_SHIFT		30
#define TCR_TG1_MASK		(UL(3) << TCR_TG1_SHIFT)
#define TCR_TG1_16K		(UL(1) << TCR_TG1_SHIFT)
#define TCR_TG1_4K		(UL(2) << TCR_TG1_SHIFT)
#define TCR_TG1_64K		(UL(3) << TCR_TG1_SHIFT)

#define TCR_IPS_SHIFT		32
#define TCR_IPS_MASK		(UL(7) << TCR_IPS_SHIFT)
#define TCR_PS_BITS_4GB		0x0ULL
#define TCR_PS_BITS_64GB	0x1ULL
#define TCR_PS_BITS_1TB		0x2ULL
#define TCR_PS_BITS_4TB		0x3ULL
#define TCR_PS_BITS_16TB	0x4ULL
#define TCR_PS_BITS_256TB	0x5ULL

#define TCR_A1			(UL(1) << 22)
#define TCR_ASID16		(UL(1) << 36)
#define TCR_TBI0		(UL(1) << 37)
#define TCR_TBI1		(UL(1) << 38)
#define TCR_HA			(UL(1) << 39)
#define TCR_HD			(UL(1) << 40)
#define TCR_NFD0		(UL(1) << 53)
#define TCR_NFD1		(UL(1) << 54)

#define TCR_SMP_FLAGS	TCR_SHARED

/* PTWs cacheable, inner/outer WBWA */
#define TCR_CACHE_FLAGS	TCR_IRGN_WBWA | TCR_ORGN_WBWA
#define TCR_TG_FLAGS	TCR_TG0_4K | TCR_TG1_4K

/* Block related */
#define TTB_L1_BLOCK_ADDR_SHIFT		30
#define TTB_L1_BLOCK_ADDR_OFFSET	(1UL << TTB_L1_BLOCK_ADDR_SHIFT)
#define TTB_L1_BLOCK_ADDR_MASK		((~(TTB_L1_BLOCK_ADDR_OFFSET - 1)) & ((1UL << 48) - 1))

#define TTB_L2_BLOCK_ADDR_SHIFT		21
#define TTB_L2_BLOCK_ADDR_OFFSET	(1UL << TTB_L2_BLOCK_ADDR_SHIFT)
#define TTB_L2_BLOCK_ADDR_MASK		((~(TTB_L2_BLOCK_ADDR_OFFSET - 1)) & ((1UL << 48) - 1))

/* Table related */
#define TTB_L0_TABLE_ADDR_SHIFT		12
#define TTB_L0_TABLE_ADDR_OFFSET	(1UL << TTB_L0_TABLE_ADDR_SHIFT)
#define TTB_L0_TABLE_ADDR_MASK		((~(TTB_L0_TABLE_ADDR_OFFSET - 1)) & ((1UL << 48) - 1))

#define TTB_L1_TABLE_ADDR_SHIFT		TTB_L0_TABLE_ADDR_SHIFT
#define TTB_L1_TABLE_ADDR_OFFSET	TTB_L0_TABLE_ADDR_OFFSET
#define TTB_L1_TABLE_ADDR_MASK		TTB_L0_TABLE_ADDR_MASK

#define TTB_L2_TABLE_ADDR_SHIFT		TTB_L0_TABLE_ADDR_SHIFT
#define TTB_L2_TABLE_ADDR_OFFSET	TTB_L0_TABLE_ADDR_OFFSET
#define TTB_L2_TABLE_ADDR_MASK		TTB_L0_TABLE_ADDR_MASK

#define TTB_L3_PAGE_ADDR_SHIFT		12
#define TTB_L3_PAGE_ADDR_OFFSET		(1UL << TTB_L3_PAGE_ADDR_SHIFT)
#define TTB_L3_PAGE_ADDR_MASK		((~(TTB_L3_PAGE_ADDR_OFFSET - 1)) & ((1UL << 48) - 1))

/* Given a virtual address, get an entry offset into a page table. */
#define l0pte_index(a) ((((addr_t) a) >> TTB_I0_SHIFT) & (TTB_L0_ENTRIES - 1))
#define l1pte_index(a) ((((addr_t) a) >> TTB_I1_SHIFT) & (TTB_L1_ENTRIES - 1))
#define l2pte_index(a) ((((addr_t) a) >> TTB_I2_SHIFT) & (TTB_L2_ENTRIES - 1))
#define l3pte_index(a) ((((addr_t) a) >> TTB_I3_SHIFT) & (TTB_L3_ENTRIES - 1))

#define pte_index_to_vaddr(i0, i1, i2, i3) ((i0 << TTB_I0_SHIFT) | i1 << TTB_I1_SHIFT) | (i2 << TTB_I2_SHIFT) | (i3 << TTB_I3_SHIFT))

#define l0pte_offset(pgtable, addr)     ((u64 *) (pgtable + l0pte_index(addr)))
#define l1pte_offset(l0pte, addr)	((u64 *) (__va(*l0pte & TTB_L0_TABLE_ADDR_MASK)) + l1pte_index(addr))
#define l2pte_offset(l1pte, addr)	((u64 *) (__va(*l1pte & TTB_L1_TABLE_ADDR_MASK)) + l2pte_index(addr))
#define l3pte_offset(l2pte, addr)	((u64 *) (__va(*l2pte & TTB_L2_TABLE_ADDR_MASK)) + l3pte_index(addr))

#define l1pte_first(l0pte)		((u64 *) __va(*l0pte & TTB_L0_TABLE_ADDR_MASK))
#define l2pte_first(l1pte)		((u64 *) __va(*l1pte & TTB_L1_TABLE_ADDR_MASK))
#define l3pte_first(l2pte)		((u64 *) __va(*l2pte & TTB_L2_TABLE_ADDR_MASK))

#define l0_addr_end(addr, end)                                         \
 ({      unsigned long __boundary = ((addr) + SZ_256G) & BLOCK_256G_MASK;  \
         (__boundary - 1 < (end) - 1) ? __boundary : (end);                \
 })

#define l1_addr_end(addr, end)                                         \
 ({      unsigned long __boundary = ((addr) + SZ_1G) & BLOCK_1G_MASK;  \
         (__boundary - 1 < (end) - 1) ? __boundary : (end);                \
 })

#define l2_addr_end(addr, end)                                         \
 ({      unsigned long __boundary = ((addr) + SZ_2M) & BLOCK_2M_MASK;  \
         (__boundary - 1 < (end) - 1) ? __boundary : (end);                \
 })


#define clear_page(page)	memset((void *)(page), 0, PAGE_SIZE)

#define PFN_DOWN(x)   ((x) >> PAGE_SHIFT)
#define PFN_UP(x)     (((x) + PAGE_SIZE-1) >> PAGE_SHIFT)

#ifndef __ASSEMBLY__

/* These constants need to be synced to the MT_ types */
enum dcache_option {
	DCACHE_OFF = MT_DEVICE_nGnRnE,
	DCACHE_WRITETHROUGH = MT_NORMAL_NC,
	DCACHE_WRITEBACK = MT_NORMAL,
	DCACHE_WRITEALLOC = MT_NORMAL,
};

static inline void set_pte_table(u64 *pte, enum dcache_option option)
{
	u64 attrs = PTE_TABLE_NS;

	*pte |= PTE_TYPE_TABLE;
	*pte |= attrs;
}

static inline void set_pte_block(u64 *pte, enum dcache_option option)
{
	u64 attrs = PTE_BLOCK_MEMTYPE(option);

	/* Permissions of R/W/Executable will be set in create_mapping() function
	 * according to the VA. The combination of UXN/PXN/AP[2:1]/SCTLR_ELx.WXN
	 * determines the level of access permission. It is not possible
	 * to have R/W/Exec at EL0/EL1 at the same time.
	 */

	*pte |= PTE_TYPE_BLOCK | PTE_BLOCK_AF | PTE_BLOCK_INNER_SHARE | PTE_BLOCK_NS;
	*pte |= attrs;
}

static inline void set_pte_page(u64 *pte, enum dcache_option option)
{
	u64 attrs = PTE_BLOCK_MEMTYPE(option);

	/* Permissions of R/W/Executable will be set in create_mapping() function
	 * according to the VA. The combination of UXN/PXN/AP[2:1]/SCTLR_ELx.WXN
	 * determines the level of access permission. It is not possible
	 * to have R/W/Exec at EL0/EL1 at the same time.
	 */

	*pte |= PTE_TYPE_PAGE | PTE_BLOCK_AF | PTE_BLOCK_INNER_SHARE | PTE_BLOCK_NS;
	*pte |= attrs;
}

static inline int pte_type(u64 *pte)
{
	return *pte & PTE_TYPE_MASK;
}

#define cpu_get_l0pgtable()	\
({						\
	unsigned long ttbr;			\
	__asm__("mrs	%0, ttbr1_el1"	\
		 : "=r" (ttbr) : : "cc");	\
	ttbr &= TTBR0_BASE_ADDR_MASK;		\
})

#define cpu_get_ttbr0() \
({						\
	unsigned long ttbr;			\
	__asm__("mrs	%0, ttbr1_el1"	\
		 : "=r" (ttbr) : : "cc");		\
	ttbr;					\
})

static inline unsigned int get_sctlr(void)
{
	unsigned int val;

	asm volatile("mrs %0, sctlr_el1" : "=r" (val) : : "cc");

	return val;
}

static inline void set_sctlr(unsigned int val)
{
	asm volatile("msr sctlr_el1, %0" : : "r" (val) : "cc");
	asm volatile("isb");
}

extern addr_t __sys_root_pgtable[], __sys_idmap_l1pgtable[], __sys_linearmap_l1pgtable[];

void *current_pgtable(void);

extern void *__current_pgtable;

static inline void set_pgtable(void *pgtable) {
	__current_pgtable = pgtable;
}

void set_pte(addr_t *pte, enum dcache_option option);

extern void __mmu_switch(void *root_pgtable_phys);

void *current_pgtable(void);
void *new_root_pgtable(void);
void copy_root_pgtable(void *dst, void *src);
void reset_root_pgtable(void *pgtable, bool remove);

addr_t virt_to_phys_pt(addr_t vaddr);

void pgtable_copy_kernel_area(void *l1pgtable);

void create_mapping(void *pgtable, addr_t virt_base, addr_t phys_base, size_t size, bool nocache);
void release_mapping(void *pgtable, addr_t virt_base, addr_t size);

void reset_l1pgtable(void *l1pgtable, bool remove);

void clear_l1pte(void *l1pgtable, addr_t vaddr);

void mmu_switch(void *l0pgtable);
void dump_pgtable(void *l0pgtable);

void dump_current_pgtable(void);

void mmu_setup(void *pgtable);

void vectors_init(void);

void set_current_pgtable(void *pgtable);

#endif


#endif /* MMU_H */

