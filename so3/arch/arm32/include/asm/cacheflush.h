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

#ifndef CACHEFLUSH_H
#define CACHEFLUSH_H

#include <asm/processor.h>

/*
 *	flush_pte_entry
 *
 *	Flush a PTE entry (word aligned, or double-word aligned) to
 *	RAM if the TLB for the CPU we are running on requires this.
 *	This is typically used when we are creating or removing PTE entries.
 *
 */
static inline void flush_pte_entry(void *pte)
{
	do {
		asm("mcr p15, 0, %0, c7, c10, 1   @ flush pte" : : "r" (pte) : "cc");
	} while (0);

	dsb();
	isb();
}

void invalidate_dcache_all(void);
void flush_dcache_all(void);
void invalidate_dcache_range(unsigned long start, unsigned long end);

void __asm_flush_dcache_range(unsigned long start, unsigned long end);

void __asm_invalidate_tlb_all(void);

void arm_init_before_mmu(void);

void mmu_page_table_flush(unsigned long start, unsigned long end);

void invalidate_icache_all(void);

void cache_enable(uint32_t cache_bit);
void cache_disable(uint32_t cache_bit);
void icache_enable(void);
void icache_disable(void);
int icache_status(void);
void dcache_enable(void);
void dcache_disable(void);
int dcache_status(void);

#endif /* CACHEFLUSH_H */
