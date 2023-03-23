// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 *
 * (C) Copyright 2016
 * Alexander Graf <agraf@suse.de>
 */

#include <types.h>

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/cacheflush.h>

/*
 *  With 4k page granule, a virtual address is split into 4 lookup parts
 *  spanning 9 bits each:
 *
 *    _______________________________________________
 *   |       |       |       |       |       |       |
 *   |   0   |  Lv0  |  Lv1  |  Lv2  |  Lv3  |  off  |
 *   |_______|_______|_______|_______|_______|_______|
 *     63-48   47-39   38-30   29-21   20-12   11-00
 *
 *             mask        page size
 *
 *    Lv0: FF8000000000       --
 *    Lv1:   7FC0000000       1G
 *    Lv2:     3FE00000       2M
 *    Lv3:       1FF000       4K
 *    off:          FFF
 */

/**
 * Activate the MMU and setup the CPU with the right attributes.
 *
 * @param pgtable	Physical address of the main L0 page table
 */
void mmu_setup(void *pgtable)
{
	u64 attr, tcr;

	tcr = TCR_CACHE_FLAGS | TCR_SMP_FLAGS | TCR_TG_FLAGS | TCR_ASID16 | TCR_A1;

#ifdef CONFIG_VA_BITS_48
	 tcr |= TCR_TxSZ(48) | (TCR_PS_BITS_256TB << TCR_IPS_SHIFT);
#elif CONFIG_VA_BITS_39
	 tcr |= TCR_TxSZ(39) | (TCR_PS_BITS_1TB << TCR_IPS_SHIFT);
#else
#error "Wrong VA_BITS configuration."
#endif

#ifdef CONFIG_ARM64VT
	asm volatile("msr tcr_el2, %0" : : "r" (tcr) : "memory");

	/* Prepare the stage-2 configuration */
	tcr =  VTCR_T0SZ_VAL(48) |
		VTCR_SL0_L0 |
		TCR_PS_BITS_256TB |
		(TCR_ORGN0_WBWA << TCR_IRGN0_SHIFT) |
		(TCR_ORGN0_WBWA << TCR_ORGN0_SHIFT) |
		TCR_SH0_INNER |
		VTCR_RES1;

	asm volatile("msr vtcr_el2, %0" : : "r" (tcr) : "memory");
	asm volatile("isb");

	attr = MAIR_EL2_SET;

	asm volatile("dsb sy");

	asm volatile("msr mair_el2, %0" : : "r" (attr) : "memory");

	asm volatile("isb");

	/* Enable the mmu and set the sctlr & ttbr register correctly. */
	__mmu_setup(pgtable);

#else /* !CONFIG_ARM64VT */

	attr = MAIR_EL1_SET;

	asm volatile("dsb sy");

	/* We need ttbr0 for mapping the devices which physical addresses
	 * are in the user space range.
	 */
	asm volatile("msr ttbr0_el1, %0" : : "r" (pgtable) : "memory");

	/* For kernel mapping */
	asm volatile("msr ttbr1_el1, %0" : : "r" (pgtable) : "memory");

	asm volatile("msr tcr_el1, %0" : : "r" (tcr) : "memory");

	asm volatile("msr mair_el1, %0" : : "r" (attr) : "memory");

	asm volatile("isb");

	/* Enable the mmu and set the sctlr register correctly. */

	set_sctlr(SCTLR_EL1_SET);

	asm volatile("isb");

	invalidate_dcache_all();
	__asm_invalidate_tlb_all();

#endif /* !CONFIG_ARM64VT */

}

/*
 * Performs a invalidation of the entire data cache at all levels
 */
void invalidate_dcache_all(void)
{
	__asm_invalidate_dcache_all(0);
}

/*
 * Performs a clean & invalidation of the entire data cache at all levels.
 * This function needs to be inline to avoid using stack.
 * __asm_flush_l3_dcache return status of timeout
 */
inline void flush_dcache_all(void)
{
	__asm_flush_dcache_all(0);
}

/*
 * Flush all TLBs on local CPU
 */
inline void flush_tlb_all(void) {
	__flush_tlb_all();
}

/*
 * Invalidates range in all levels of D-cache/unified cache
 */
void invalidate_dcache_range(unsigned long start, unsigned long end)
{
	__asm_invalidate_dcache_range(start, end);
}

/*
 * Flush range(clean & invalidate) from all levels of D-cache/unified cache
 */
void flush_dcache_range(unsigned long start, unsigned long end)
{
	__asm_flush_dcache_range(start, end);
}

/*
 * Flush an individual PTE entry
 */
void flush_pte_entry(addr_t va, u64 *pte) {
	__asm_invalidate_tlb(va);
	invalidate_dcache_range((u64) pte, (u64) (pte+1));

}

void dcache_enable(void)
{
	set_sctlr(get_sctlr() | CR_C);
}

void dcache_disable(void)
{
	uint32_t sctlr;

	sctlr = get_sctlr();

	/* if cache isn't enabled no need to disable */
	if (!(sctlr & CR_C))
		return;

	set_sctlr(sctlr & ~(CR_C|CR_M));

	flush_dcache_all();
	__asm_invalidate_tlb_all();
}

int dcache_status(void)
{
	return (get_sctlr() & CR_C) != 0;
}

void icache_enable(void)
{
	set_sctlr(get_sctlr() | CR_I);
}

void icache_disable(void)
{
	set_sctlr(get_sctlr() & ~CR_I);
}

int icache_status(void)
{
	return (get_sctlr() & CR_I) != 0;
}

void invalidate_icache_all(void)
{
	__asm_invalidate_icache_all();
}

void mmu_page_table_flush(unsigned long start, unsigned long end) {
	flush_dcache_range(start, end);
	flush_tlb_all();
	__asm_invalidate_tlb_all();
}

