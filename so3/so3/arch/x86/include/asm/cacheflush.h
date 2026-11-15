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

#ifndef ASM_CACHEFLUSH_H
#define ASM_CACHEFLUSH_H

#include <types.h>

#include <asm/processor.h>

/*
 * x86 caches are coherent with page table walks and DMA, so the ARM cache
 * maintenance calls used by the generic code reduce to TLB handling.
 */

static inline void flush_dcache_all(void)
{
}

static inline void flush_dcache_range(addr_t start, addr_t end)
{
}

static inline void invalidate_dcache_range(addr_t start, addr_t end)
{
}

static inline void invalidate_dcache_all(void)
{
}

static inline void invalidate_icache_all(void)
{
}

static inline void mmu_page_table_flush(unsigned long start, unsigned long end)
{
}

static inline void flush_pte_entry(addr_t va, u64 *pte)
{
	invlpg(va);
}

static inline void flush_tlb_all(void)
{
	write_cr3(read_cr3());
}

#define __asm_invalidate_tlb_all() flush_tlb_all()

#endif /* ASM_CACHEFLUSH_H */
