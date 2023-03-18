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

#ifndef ASM_SETUP_H
#define ASM_SETUP_H

#define L_TEXT_OFFSET	0x80000

/*
 * Memory map description
 */
#define NR_BANKS 8

struct meminfo {
	int nr_banks;
	unsigned long end;
	struct {
		unsigned long start;
		unsigned long size;
		int           node;
	} bank[NR_BANKS];
};

extern struct meminfo meminfo;

extern addr_t __cpu1_stack[];
extern addr_t __cpu3_stack[];

void setup_arch(void);
void cpu_init(void);

#endif /* ASM_SETUP_H */
