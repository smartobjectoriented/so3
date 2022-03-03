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

#ifndef ASM_MEMORY_H
#define ASM_MEMORY_H

#include <generated/autoconf.h>

#ifndef __ASSEMBLY__
#include <types.h>

extern uint32_t *__sys_l1pgtable;

extern page_t *frame_table;
extern uint32_t pfn_start;

#define pfn_to_phys(pfn) ((pfn) << PAGE_SHIFT)
#define phys_to_pfn(phys) (((uint32_t) phys) >> PAGE_SHIFT)
#define virt_to_pfn(virt) (phys_to_pfn(__va((uint32_t) virt)))

#define __pa(vaddr) (((uint32_t) vaddr) - CONFIG_KERNEL_VIRT_ADDR + ((uint32_t) CONFIG_RAM_BASE))
#define __va(paddr) (((uint32_t) paddr) - ((uint32_t) CONFIG_RAM_BASE) + CONFIG_KERNEL_VIRT_ADDR)

#define page_to_pfn(page) ((uint32_t) ((uint32_t) (page-frame_table) + pfn_start))
#define pfn_to_page(pfn) (&frame_table[pfn - pfn_start])

#define page_to_phys(page) (pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys) (pfn_to_page(phys_to_pfn(phys)))


#endif /* __ASSEMBLY__ */

#endif /* ASM_MEMORY_H */
