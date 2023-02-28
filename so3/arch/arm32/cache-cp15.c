// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#if 0
#define DEBUG
#endif

#include <common.h>
#include <types.h>

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/cacheflush.h>

#define CONFIG_SYS_CACHELINE_SIZE 64

#ifdef CONFIG_MMU

void set_l1_pte_sect_dcache(uint32_t *l1pte, enum ttb_l1_sect_dcache_option option)
{
	u32 value;

	/* Reset all bits out of addr bits */
	*l1pte = *l1pte & TTB_L1_SECT_ADDR_MASK;

	/* Set permission bits */
	value = TTB_L1_AP;

	/* Add caching bits */
	value |= option;

	/* Set PTE */
	*l1pte |= value;
}

void set_l1_pte_page_dcache(uint32_t *l1pte, enum ttb_l1_page_dcache_option option)
{
	u32 value = 0;

	/* Reset all bits out of addr bits */
	*l1pte = *l1pte & TTB_L1_PAGE_ADDR_MASK;

	/* Add caching bits */
	value |= option;

	/* Set PTE */
	*l1pte |= value;
}

void set_l2_pte_dcache(uint32_t *l2pte, enum ttb_l2_dcache_option option)
{
	u32 value;

	/* Reset all bits out of addr bits */
	*l2pte = *l2pte & TTB_L2_ADDR_MASK;

	/* Set permission bits */
	value = TTB_L2_AP;

	/* Reset all bits out of addr bits */
	*l2pte = *l2pte & TTB_L2_ADDR_MASK;

	/* Add caching bits */
	value |= option;

	/* Set PTE */
	*l2pte |= value;
}

/* To activate the MMU we need to set up virtual memory */
void mmu_setup(void *pgtable)
{
	u32 reg;

	arm_init_before_mmu();

	/* Set TTBCR to disable LPAE */
	asm volatile("mcr p15, 0, %0, c2, c0, 2" : : "r" (0) : "memory");

	/* Set TTBR0 */
	reg = ((addr_t) pgtable) & TTBR0_BASE_ADDR_MASK;
	reg |= TTBR0_RGN_WBWA | TTBR0_IRGN_WBWA;

	asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r" (reg) : "memory");

	/* Set the access control to all-supervisor */
	asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r" (~0));

	arm_init_domains();

	/* and enable the mmu */
	reg = get_cr();	/* get control reg. */
	set_cr(reg | CR_V | CR_M);
}

int mmu_enabled(void)
{
	return get_cr() & CR_M;
}

#endif /* CONFIG_MMU */

/* cache_bit must be either CR_I or CR_C */
void cache_enable(uint32_t cache_bit)
{
	uint32_t reg;

	/* The data cache is not active unless the mmu is enabled too */

	reg = get_cr();	/* get control reg. */
	set_cr(reg | cache_bit);
}

/* cache_bit must be either CR_I or CR_C */
void cache_disable(uint32_t cache_bit)
{
	uint32_t reg;

	reg = get_cr();

	if (cache_bit == CR_C) {
		/* if cache isn;t enabled no need to disable */
		if ((reg & CR_C) != CR_C)
			return;
		/* if disabling data cache, disable mmu too */
		cache_bit |= CR_M;
	}
	reg = get_cr();

	if (cache_bit == (CR_C | CR_M))
		flush_dcache_all();

	set_cr(reg & ~cache_bit);
}


void icache_enable(void)
{
	cache_enable(CR_I);
}

void icache_disable(void)
{
	cache_disable(CR_I);
}

int icache_status(void)
{
	return (get_cr() & CR_I) != 0;
}

void dcache_enable(void)
{
	cache_enable(CR_C | CR_W);
}

void dcache_disable(void)
{
	cache_disable(CR_C | CR_W);
}

int dcache_status(void)
{
	return (get_cr() & CR_C) != 0;
}

