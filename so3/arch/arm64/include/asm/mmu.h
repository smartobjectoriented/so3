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

#include <generated/autoconf.h>

#ifndef __ASSEMBLY__
#include <types.h>
#endif

#include <sizes.h>

#define USER_SPACE_VADDR	UL(0x1000)

#define RAMDEV_VADDR		UL(0xffffa00000000000)

/* Fixmap page used for temporary mapping */
#define FIXMAP_MAPPING		UL(0xffffb00000000000)

/* The user space can be up to bits [47:0] and uses ttbr0_el1
 * as main L0 page table.
 */

#define USER_STACK_TOP_VADDR	UL(0x0001000000000000)

#define	SZ_256G		(256UL * SZ_1G)

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE       (1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

#ifdef CONFIG_ARM64VT

#ifdef CONFIG_VA_BITS_48
#define AGENCY_VOFFSET		UL(0x0000110000000000)
#define ME_VOFFSET		UL(0x0000200100000000)
#elif CONFIG_VA_BITS_39
#define AGENCY_VOFFSET		UL(0xffffffc010000000)
#define ME_VOFFSET	  	UL(0xffffffc000000000)
#else
#error "Wrong VA_BITS configuration."
#endif

#else /* CONFIG_ARM64VT */

#ifdef CONFIG_VA_BITS_48
#define AGENCY_VOFFSET	UL(0xffff800010000000)
#define ME_VOFFSET	UL(0xffff800010000000)
#elif CONFIG_VA_BITS_39
#define AGENCY_VOFFSET	UL(0xffffffc010000000)
#define ME_VOFFSET  	UL(0xffffffc000000000)
#else
#error "Wrong VA_BITS configuration."
#endif

#endif /* !CONFIG_ARM64VT */

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

#define MAIR_EL2_SET							\
	(MAIR_ATTRIDX(MAIR_ATTR_DEVICE_nGnRnE, MT_DEVICE_nGnRnE) |	\
	 MAIR_ATTRIDX(MAIR_ATTR_DEVICE_nGnRE, MT_DEVICE_nGnRE) |	\
	 MAIR_ATTRIDX(MAIR_ATTR_DEVICE_GRE, MT_DEVICE_GRE) |		\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL_NC, MT_NORMAL_NC) |		\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL, MT_NORMAL) |			\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL_WT, MT_NORMAL_WT) |		\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL, MT_NORMAL_TAGGED))

#ifndef __ASSEMBLY__

/* These constants need to be synced to the MT_ types */
enum dcache_option {
	DCACHE_OFF = MT_DEVICE_nGnRnE,
	DCACHE_WRITETHROUGH = MT_NORMAL_NC,
	DCACHE_WRITEBACK = MT_NORMAL,
	DCACHE_WRITEALLOC = MT_NORMAL,
};

#endif

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
#define PTE_TYPE_FLAG_TERMINAL	(1 << 1)

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

/*
 * When combining shareability attributes, the stage-1 ones prevail. So we can
 * safely leave everything non-shareable at stage 2.
 */

#define PTE_BLOCK_NON_SHARE	(0UL << 8)
#define PTE_BLOCK_OUTER_SHARE	(2UL << 8)
#define PTE_BLOCK_INNER_SHARE	(3UL << 8)

#define PTE_BLOCK_AF		(1UL << 10)
#define PTE_BLOCK_NG		(1UL << 11)
#define PTE_BLOCK_DBM		(1UL << 51)
#define PTE_BLOCK_PXN		(1UL << 53)
#define PTE_BLOCK_UXN		(1UL << 54)

/*
 * Stage-1 and Stage-2 lower attributes.
 * FIXME: The upper attributes (contiguous hint and XN) are not currently in
 * use. If needed in the future, they should be shifted towards the lower word,
 * since the core uses unsigned long to pass the flags.
 * An arch-specific typedef for the flags as well as the addresses would be
 * useful.
 * The contiguous bit is a hint that allows the PE to store blocks of 16 pages
 * in the TLB. This may be a useful optimisation.
 */

/* These bits differ in stage 1 and 2 translations */
#define S1_PTE_NG		(0x1 << 11)
#define S1_PTE_ACCESS_RW	(0x0 << 7)
#define S1_PTE_ACCESS_RO	(0x1 << 7)

/* Res1 for EL2 stage-1 tables */
#define S1_PTE_ACCESS_EL0	(0x1 << 6)

#define S2_PTE_ACCESS_RO	(0x1 << 6)
#define S2_PTE_ACCESS_WO	(0x2 << 6)
#define S2_PTE_ACCESS_RW	(0x3 << 6)

#define VTTBR_VMID_SHIFT	48

#define HTCR_RES1		((UL(1) << 31) | (UL(1) << 23))
#define VTCR_RES1		((UL(1) << 31))

/* Stage 2 memory attributes (MemAttr[3:0]) */
#define S2_MEMATTR_OWBIWB	0xf
#define S2_MEMATTR_DEV		0x1

#define S2_PTE_FLAG_NORMAL	PTE_BLOCK_MEMTYPE(S2_MEMATTR_OWBIWB)
#define S2_PTE_FLAG_DEVICE	PTE_BLOCK_MEMTYPE(S2_MEMATTR_DEV)

#define S1_DEFAULT_FLAGS	(PTE_FLAG_VALID | PTE_ACCESS_FLAG	\
				| S1_PTE_FLAG_NORMAL | PTE_INNER_SHAREABLE\
				| S1_PTE_ACCESS_EL0)

/* Macros used by the core, only for the EL2 stage-1 mappings */
#define PAGE_FLAG_FRAMEBUFFER	S1_PTE_FLAG_DEVICE
#define PAGE_FLAG_DEVICE	S1_PTE_FLAG_DEVICE
#define PAGE_DEFAULT_FLAGS	(S1_DEFAULT_FLAGS | S1_PTE_ACCESS_RW)
#define PAGE_READONLY_FLAGS	(S1_DEFAULT_FLAGS | S1_PTE_ACCESS_RO)
#define PAGE_PRESENT_FLAGS	PTE_FLAG_VALID
#define PAGE_NONPRESENT_FLAGS	0

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

#define TCR_EL2_RES1		((1 << 31) | (1 << 23))
#define T0SZ(parange)		(64 - parange)
#define SL0_L0			2
#define SL0_L1			1
#define SL0_L2			0
#define PARANGE_48B		0x5
#define TCR_RGN_NON_CACHEABLE	0x0
#define TCR_RGN_WB_WA		0x1
#define TCR_RGN_WT		0x2
#define TCR_RGN_WB		0x3
#define TCR_NON_SHAREABLE	0x0
#define TCR_OUTER_SHAREABLE	0x2
#define TCR_INNER_SHAREABLE	0x3

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

#define TCR_PS_SHIFT		16
#define TCR_IRGN0_SHIFT		8
#define TCR_SL0_SHIFT		6
#define TCR_S_SHIFT		4

#define TCR_RGN_NON_CACHEABLE	0x0

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
#define TCR_PS_SHIFT		16

#define TCR_A1			(UL(1) << 22)
#define TCR_ASID16		(UL(1) << 36)
#define TCR_TBI0		(UL(1) << 37)
#define TCR_TBI1		(UL(1) << 38)
#define TCR_HA			(UL(1) << 39)
#define TCR_HD			(UL(1) << 40)
#define TCR_NFD0		(UL(1) << 53)
#define TCR_NFD1		(UL(1) << 54)

#define TCR_SMP_FLAGS	TCR_SHARED

/* VTTBR */
#define VTTBR_INITVAL					0x0000000000000000ULL
#define VTTBR_VMID_MASK					0x00FF000000000000ULL
#define VTTBR_VMID_SHIFT				48
#define VTTBR_BADDR_MASK				0x000000FFFFFFF000ULL
#define VTTBR_BADDR_SHIFT				12

/* VTCR_EL2 */
#define VTCR_INITVAL					0x80000000
#define VTCR_PS_MASK					0x00070000
#define VTCR_PS_SHIFT					16
#define VTCR_TG0_MASK					0x0000c000
#define VTCR_TG0_SHIFT					14
#define VTCR_SH0_MASK					0x00003000
#define VTCR_SH0_SHIFT					12
#define VTCR_ORGN0_MASK					0x00000C00
#define VTCR_ORGN0_SHIFT				10
#define VTCR_IRGN0_MASK					0x00000300
#define VTCR_IRGN0_SHIFT				8
#define VTCR_SL0_MASK					0x000000C0
#define VTCR_SL0_SHIFT					6
#define VTCR_T0SZ_MASK					0x0000003f
#define VTCR_T0SZ_SHIFT					0

#define VTCR_PS_32BITS					(0 << VTCR_PS_SHIFT)
#define VTCR_PS_36BITS					(1 << VTCR_PS_SHIFT)
#define VTCR_PS_40BITS					(2 << VTCR_PS_SHIFT)
#define VTCR_PS_42BITS					(3 << VTCR_PS_SHIFT)
#define VTCR_PS_44BITS					(4 << VTCR_PS_SHIFT)
#define VTCR_PS_48BITS					(5 << VTCR_PS_SHIFT)
#define VTCR_SL0_L2					(0 << VTCR_SL0_SHIFT) /* Starting-level: 2 */
#define VTCR_SL0_L1					(1 << VTCR_SL0_SHIFT) /* Starting-level: 1 */
#define VTCR_SL0_L0					(2 << VTCR_SL0_SHIFT) /* Starting-level: 0 */
#define VTCR_T0SZ_VAL(in_bits)				((64 - (in_bits)) & VTCR_T0SZ_MASK)


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

#define l0pte_offset(pgtable, addr)     ((u64 *) ((u64 *) pgtable + l0pte_index(addr)))
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

typedef enum {
	S1,
	S2
} mmu_stage_t;

#define mrs(spr)		({ u64 rval; asm volatile(\
				"mrs %0," #spr :"=r"(rval)); rval; })

#define msr(spr, val)		asm volatile("msr " #spr ", %0" ::"r"(val));

/* VA to PA Address Translation */

#define VA2PA_STAGE1		"s1"
#define VA2PA_STAGE12		"s12"
#define VA2PA_EL0		"e0"
#define VA2PA_EL1		"e1"
#define VA2PA_EL2		"e2"
#define VA2PA_EL3		"e3"
#define VA2PA_RD		"r"
#define VA2PA_WR		"w"
#define va2pa_at(stage, el, rw, va)	asm volatile(	\
					"at " stage el rw ", %0" \
					: : "r"(va) : "memory", "cc");

#ifdef CONFIG_ARM64VT

typedef struct {
	addr_t ipa_addr;
	addr_t phys_addr;
	size_t size;
} ipamap_t;

static inline void set_pte_table_S2(u64 *pte, enum dcache_option option)
{
	*pte |= PTE_TYPE_TABLE;
}

static inline void set_pte_block_S2(u64 *pte, enum dcache_option option)
{
	*pte |= PTE_TYPE_BLOCK | S2_PTE_ACCESS_RW | PTE_BLOCK_INNER_SHARE | PTE_BLOCK_AF;

	if (option == DCACHE_OFF)
		*pte |= S2_PTE_FLAG_DEVICE;
	else
		*pte |= S2_PTE_FLAG_NORMAL;
}

static inline void set_pte_page_S2(u64 *pte, enum dcache_option option)
{
	*pte |= PTE_TYPE_PAGE | PTE_BLOCK_AF | PTE_BLOCK_INNER_SHARE | S2_PTE_ACCESS_RW;

	if (option == DCACHE_OFF)
		*pte |= S2_PTE_FLAG_DEVICE;
	else
		*pte |= S2_PTE_FLAG_NORMAL;
}

#endif /* CONFIG_ARM64_VT */

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


#define cpu_get_ttbr1() \
({						\
	unsigned long ttbr;			\
	__asm__("mrs	%0, ttbr1_el1"	\
		 : "=r" (ttbr) : : "cc");		\
	ttbr;					\
})

/**
 * Check if a virtual address is within the user space range.
 *
 * @param addr	Virtual address to be checked
 * @return	true if the 16 MSB is to 0xffff, false otherwise
 */
static inline bool user_space_vaddr(addr_t addr) {
	if ((addr >> 48) & 0xffff)
		return false;
	else
		return true;
}

#ifdef CONFIG_ARM64VT

static inline unsigned int get_sctlr(void)
{
	unsigned int val;

	asm volatile("mrs %0, sctlr_el2" : "=r" (val) : : "cc");

	return val;
}

static inline void set_sctlr(unsigned int val)
{
	asm volatile("msr sctlr_el2, %0" : : "r" (val) : "cc");
	asm volatile("isb");
}

#else

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


#endif

extern addr_t __sys_root_pgtable[], __sys_idmap_l1pgtable[], __sys_linearmap_l1pgtable[], __sys_linearmap_l2pgtable[];

void *current_pgtable(void);

extern void *__current_pgtable;

static inline void set_pgtable(void *pgtable) {
	__current_pgtable = pgtable;
}
extern void __mmu_switch_ttbr1(void *root_pgtable_phys);
extern void __mmu_switch_ttbr0(void *root_pgtable_phys);

#ifdef CONFIG_AVZ

extern void __mmu_switch_ttbr0_el2(void *root_pgtable_phys);
extern void __mmu_switch_vttbr(void *root_pgtable_phys);

#endif /* CONFIG_AVZ */

void __mmu_setup(void *pgtable);

#ifdef CONFIG_ARM64VT
void do_ipamap(void *pgtable, ipamap_t ipamap[], int nbelement);
#endif

void *current_pgtable(void);
void *new_root_pgtable(void);
void copy_root_pgtable(void *dst, void *src);
void reset_root_pgtable(void *pgtable, bool remove);
void ramdev_create_mapping(void *root_pgtable, addr_t ramdev_start, addr_t ramdev_end);

addr_t virt_to_phys_pt(addr_t vaddr);

void pgtable_copy_kernel_area(void *l1pgtable);

void mmu_setup(void *pgtable);

void create_mapping(void *pgtable, addr_t virt_base, addr_t phys_base, size_t size, bool nocache);
void release_mapping(void *pgtable, addr_t virt_base, size_t size);

void *new_root_pgtable(void);

#ifdef CONFIG_AVZ

void __create_mapping(void *pgtable, addr_t virt_base, addr_t phys_base, size_t size, bool nocache, mmu_stage_t stage);
void __mmu_switch_kernel(void *pgtable, bool vttbr);

#endif /* CONFIG_AVZ */

void create_mapping(void *pgtable, addr_t virt_base, addr_t phys_base, size_t size, bool nocache);

void mmu_switch(void *pgtable_paddr);
void mmu_switch_kernel(void *pgtable);

void mmu_get_current_pgtable(addr_t *pgtable_paddr);
void dump_pgtable(void *pgtable);

void dump_current_pgtable(void);
void replace_current_pgtable_with(void *pgtable);

void vectors_init(void);

#endif /* __ASSEMBLY__ */


#endif /* MMU_H */

