/*
 * Copyright (C) 2021 Nicolas Müller <nicolas.muller1@heig-vd.ch>
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

#ifndef ARCH_RISCV64_INCLUDE_ASM_FAULT_H_
#define ARCH_RISCV64_INCLUDE_ASM_FAULT_H_

void kernel_panic(void);

void __instr_addr_misalignment(void);

void __instr_access_fault(void);

void __illegal_instr(void);

void __load_addr_misalignement(void);

void __load_access_fault(void);

void __store_AMO_addr_misaligned(void);

void __store_AMO_access_fault(void);

void __instr_page_fault(void);

void __load_page_fault(void);

void __store_AMO_page_fault(void);

#endif /* ARCH_RISCV64_INCLUDE_ASM_FAULT_H_ */
