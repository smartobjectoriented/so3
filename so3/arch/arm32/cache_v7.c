// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 * Aneesh V <aneesh@ti.com>
 */
#include <types.h>
#include <common.h>

#include <asm/processor.h>

#define ARMV7_DCACHE_INVAL_RANGE	1
#define ARMV7_DCACHE_CLEAN_INVAL_RANGE	2

#define CONFIG_SYS_CACHELINE_SIZE 64

/* Asm functions from cache_v7_asm.S */
void v7_flush_dcache_all(void);
void v7_invalidate_dcache_all(void);

static u32 get_ccsidr(void)
{
	u32 ccsidr;

	/* Read current CP15 Cache Size ID Register */
	asm volatile ("mrc p15, 1, %0, c0, c0, 0" : "=r" (ccsidr));

	return ccsidr;
}

static void v7_dcache_clean_inval_range(u32 start, u32 end, u32 line_len)
{
	u32 mva;

	/* Align start to cache line boundary */
	start &= ~(line_len - 1);
	for (mva = start; mva < end; mva = mva + line_len) {
		/* DCCIMVAC - Clean & Invalidate data cache by MVA to PoC */
		asm volatile ("mcr p15, 0, %0, c7, c14, 1" : : "r" (mva));
	}
}

int check_cache_range(unsigned long start, unsigned long end)
{
	int ok = 1;

	if (start & (CONFIG_SYS_CACHELINE_SIZE - 1))
		ok = 0;

	if (end & (CONFIG_SYS_CACHELINE_SIZE - 1))
		ok = 0;

	if (!ok) {
		lprintk("CACHE: Misaligned operation at range [%08lx, %08lx]\n", start, end);
		kernel_panic();
	}

	return ok;
}

static void v7_dcache_inval_range(u32 start, u32 end, u32 line_len)
{
	u32 mva;

	if (!check_cache_range(start, end))
		return;

	for (mva = start; mva < end; mva = mva + line_len) {
		/* DCIMVAC - Invalidate data cache by MVA to PoC */
		asm volatile ("mcr p15, 0, %0, c7, c6, 1" : : "r" (mva));
	}
}

static void v7_dcache_maint_range(u32 start, u32 end, u32 range_op)
{
	u32 line_len, ccsidr;

	ccsidr = get_ccsidr();
	line_len = ((ccsidr & CCSIDR_LINE_SIZE_MASK) >> CCSIDR_LINE_SIZE_OFFSET) + 2;

	/* Converting from words to bytes */
	line_len += 2;
	
	/* converting from log2(linelen) to linelen */
	line_len = 1 << line_len;

	switch (range_op) {
	case ARMV7_DCACHE_CLEAN_INVAL_RANGE:
		v7_dcache_clean_inval_range(start, end, line_len);
		break;
	case ARMV7_DCACHE_INVAL_RANGE:
		v7_dcache_inval_range(start, end, line_len);
		break;
	}

	/* DSB to make sure the operation is complete */
	dsb();
}

/* Invalidate TLB */
void __asm_invalidate_tlb_all(void)
{
#if 0 /* Not really necessary in our case. */
	/* Invalidate entire unified TLB */
	asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));
#endif

	/* Invalidate entire data TLB */
	asm volatile ("mcr p15, 0, %0, c8, c6, 0" : : "r" (0));

#if 0 /* Not really necessary in our case. */
	/* Invalidate entire instruction TLB */

	asm volatile ("mcr p15, 0, %0, c8, c5, 0" : : "r" (0));

	/* (Cortex-A72) Invalidate entire instruction TLB */
	asm volatile ("mcr p15, 0, %0, c8, c3, 0" : : "r" (0));
#endif

	/* Full system DSB - make sure that the invalidation is complete */
	dsb();
	/* Full system ISB - make sure the instruction stream sees it */
	isb();
}

void invalidate_dcache_all(void)
{
	v7_invalidate_dcache_all();
}

/*
 * Performs a clean & invalidation of the entire data cache
 * at all levels
 */
void flush_dcache_all(void)
{
	v7_flush_dcache_all();
}

/*
 * Invalidates range in all levels of D-cache/unified cache used:
 * Affects the range [start, end - 1]
 */
void invalidate_dcache_range(unsigned long start, unsigned long end)
{
	check_cache_range(start, end);

	v7_dcache_maint_range(start, end, ARMV7_DCACHE_INVAL_RANGE);
}

/*
 * Flush range(clean & invalidate) from all levels of D-cache/unified
 * cache used:
 * Affects the range [start, end - 1]
 */
void __asm_flush_dcache_range(addr_t start, addr_t end)
{
	check_cache_range(start, end);

	v7_dcache_maint_range(start, end, ARMV7_DCACHE_CLEAN_INVAL_RANGE);
}

void arm_init_before_mmu(void)
{
	invalidate_dcache_all();
	__asm_invalidate_tlb_all();
}

void mmu_page_table_flush(unsigned long start, unsigned long end)
{
	__asm_flush_dcache_range(start, end);
	__asm_invalidate_tlb_all();
}

/* Invalidate entire I-cache and branch predictor array */
void invalidate_icache_all(void)
{
	/*
	 * Invalidate all instruction caches to PoU.
	 * Also flushes branch target cache.
	 */
	asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));

	/* Invalidate entire branch predictor array */
	asm volatile ("mcr p15, 0, %0, c7, c5, 6" : : "r" (0));

	/* Branch predictor invalidate all Inner Shareable */
	asm volatile ("mcr p15, 0, %0, c7, c1, 6" : : "r" (0));

	/* Full system DSB - make sure that the invalidation is complete */
	dsb();

	/* ISB - make sure the instruction stream sees it */
	isb();
}

