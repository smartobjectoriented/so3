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

void flush_pte_entry(addr_t va, u64 *pte);

void mmu_page_table_flush(unsigned long start, unsigned long end);

void __asm_invalidate_tlb_all(void);
void __asm_invalidate_tlb(addr_t va);
void __asm_dcache_level(int level);
void __asm_invalidate_dcache_range(addr_t start, addr_t end);
void __asm_flush_dcache_range(addr_t start, addr_t end);
void __asm_invalidate_icache_all(void);

void __asm_flush_dcache_all(int invalidate_only);
void __asm_invalidate_dcache_all(int invalidate_only);
void __flush_tlb_all(void);

void invalidate_dcache_all(void);
void invalidate_icache_all(void);
inline void flush_dcache_all(void);
void flush_tlb_all(void);

void cache_enable(uint32_t cache_bit);
void cache_disable(uint32_t cache_bit);
void icache_enable(void);
void icache_disable(void);
int icache_status(void);
void dcache_enable(void);
void dcache_disable(void);
int dcache_status(void);

#endif /* CACHEFLUSH_H */
