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

#ifndef MMU_H
#define MMU_H

#include <generated/autoconf.h>

#ifndef __ASSEMBLY__
#include <types.h>
#endif

#include <sizes.h>
#include <linkage.h>

#define USER_SPACE_VADDR UL(0x1000)

#define RAMDEV_VADDR UL(0xffffa00000000000)

/* Fixmap page used for temporary mapping */

#define FIXMAP_MAPPING UL(0xffffb00000000000)

/* Anonymous start virtual address */

#define USER_ANONYMOUS_VADDR UL(0x0000000100000000)

/*
 * The user space is the lower canonical half, below 0x0000_8000_0000_0000.
 * The stack top stays well inside it.
 */

#define USER_STACK_TOP_VADDR UL(0x0000700000000000)

#define SZ_256G (256UL * SZ_1G)

/* PAGE_SHIFT determines the page size */

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define BLOCK_256G_OFFSET (SZ_256G - 1)
#define BLOCK_1G_OFFSET (SZ_1G - 1)
#define BLOCK_2M_OFFSET (SZ_2M - 1)

#define BLOCK_256G_MASK (~BLOCK_256G_OFFSET)
#define BLOCK_1G_MASK (~BLOCK_1G_OFFSET)
#define BLOCK_2M_MASK (~BLOCK_2M_OFFSET)

/*
 * 4-level paging: L0 = PML4, L1 = PDPT, L2 = PD, L3 = PT. The geometry is
 * the one of the AArch64 4 KB granule, so the names are kept.
 */

#define TTB_I0_SHIFT 39
#define TTB_I1_SHIFT 30
#define TTB_I2_SHIFT 21
#define TTB_I3_SHIFT 12

#define TTB_L0_ENTRIES 512
#define TTB_L1_ENTRIES 512
#define TTB_L2_ENTRIES 512
#define TTB_L3_ENTRIES 512

#define TTB_L0_SIZE 4096
#define TTB_L1_SIZE 4096
#define TTB_L2_SIZE 4096
#define TTB_L3_SIZE 4096

/* First L0 entry of the kernel (upper) half, shared by all page tables */

#define TTB_L0_KERNEL_FIRST 256

#define PTE_ADDR_MASK UL(0x000ffffffffff000)

#define TTB_L0_TABLE_ADDR_MASK PTE_ADDR_MASK
#define TTB_L1_TABLE_ADDR_MASK PTE_ADDR_MASK
#define TTB_L2_TABLE_ADDR_MASK PTE_ADDR_MASK
#define TTB_L3_PAGE_ADDR_MASK PTE_ADDR_MASK

#define TTB_L1_BLOCK_ADDR_MASK UL(0x000fffffc0000000)
#define TTB_L2_BLOCK_ADDR_MASK UL(0x000fffffffe00000)

#define PTE_PRESENT (UL(1) << 0)
#define PTE_RW (UL(1) << 1)
#define PTE_USER (UL(1) << 2)
#define PTE_PWT (UL(1) << 3)
#define PTE_PCD (UL(1) << 4)
#define PTE_ACCESSED (UL(1) << 5)
#define PTE_DIRTY (UL(1) << 6)
#define PTE_PS (UL(1) << 7)
#define PTE_GLOBAL (UL(1) << 8)
#define PTE_NX (UL(1) << 63)

#define PTE_TYPE_TABLE 1
#define PTE_TYPE_BLOCK 2
#define PTE_TYPE_PAGE 3

#define l0pte_index(a) ((((addr_t) a) >> TTB_I0_SHIFT) & (TTB_L0_ENTRIES - 1))
#define l1pte_index(a) ((((addr_t) a) >> TTB_I1_SHIFT) & (TTB_L1_ENTRIES - 1))
#define l2pte_index(a) ((((addr_t) a) >> TTB_I2_SHIFT) & (TTB_L2_ENTRIES - 1))
#define l3pte_index(a) ((((addr_t) a) >> TTB_I3_SHIFT) & (TTB_L3_ENTRIES - 1))

#define l0pte_offset(pgtable, addr) ((u64 *) ((u64 *) pgtable + l0pte_index(addr)))
#define l1pte_offset(l0pte, addr) ((u64 *) (__va(*l0pte & TTB_L0_TABLE_ADDR_MASK)) + l1pte_index(addr))
#define l2pte_offset(l1pte, addr) ((u64 *) (__va(*l1pte & TTB_L1_TABLE_ADDR_MASK)) + l2pte_index(addr))
#define l3pte_offset(l2pte, addr) ((u64 *) (__va(*l2pte & TTB_L2_TABLE_ADDR_MASK)) + l3pte_index(addr))

#define l0_addr_end(addr, end)                                                   \
	({                                                                       \
		unsigned long __boundary = ((addr) + SZ_256G) & BLOCK_256G_MASK; \
		(__boundary - 1 < (end) - 1) ? __boundary : (end);               \
	})

#define l1_addr_end(addr, end)                                               \
	({                                                                   \
		unsigned long __boundary = ((addr) + SZ_1G) & BLOCK_1G_MASK; \
		(__boundary - 1 < (end) - 1) ? __boundary : (end);           \
	})

#define l2_addr_end(addr, end)                                               \
	({                                                                   \
		unsigned long __boundary = ((addr) + SZ_2M) & BLOCK_2M_MASK; \
		(__boundary - 1 < (end) - 1) ? __boundary : (end);           \
	})

#define clear_page(page) memset((void *) (page), 0, PAGE_SIZE)

#define PFN_DOWN(x) ((x) >> PAGE_SHIFT)
#define PFN_UP(x) (((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)

#ifndef __ASSEMBLY__

static inline bool user_space_vaddr(addr_t addr)
{
	return addr < UL(0x0000800000000000);
}

/* Type of a present entry at level 0 (PML4) to 3 (PT) */

static inline int pte_type(u64 *pte, int level)
{
	if (level == 3)
		return PTE_TYPE_PAGE;

	if ((level == 1 || level == 2) && (*pte & PTE_PS))
		return PTE_TYPE_BLOCK;

	return PTE_TYPE_TABLE;
}

extern addr_t __sys_root_pgtable[], __sys_idmap_l1pgtable[], __sys_linearmap_l1pgtable[], __sys_linearmap_l2pgtable[];

extern void *__current_pgtable;

void *current_pgtable(void);

static inline void set_pgtable(void *pgtable)
{
	__current_pgtable = pgtable;
}

void *new_root_pgtable(void);
void copy_root_pgtable(void *dst, void *src);
void reset_root_pgtable(void *pgtable, bool remove);
void pgtable_copy_kernel_area(void *pgtable);

void ramdev_create_mapping(void *root_pgtable, addr_t ramdev_start, addr_t ramdev_end);

addr_t virt_to_phys_pt(addr_t vaddr);

void create_mapping(void *pgtable, addr_t virt_base, addr_t phys_base, size_t size, bool nocache);
void release_mapping(void *pgtable, addr_t virt_base, size_t size);

void mmu_switch(void *pgtable_paddr);
void mmu_switch_kernel(void *pgtable);

void dump_pgtable(void *pgtable);

struct pcb;
void duplicate_user_space(struct pcb *from, struct pcb *to);

#endif /* __ASSEMBLY__ */

#endif /* MMU_H */
