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

#ifndef ARM_TIMER_H
#define ARM_TIMER_H

#include <types.h>
#include <timer.h>

#include <asm/processor.h>
#include <asm/csr.h>


static inline u32 arch_timer_get_cntfrq(void)
{
	return 10000000UL;
}

static inline u64 arch_get_time(void) {

		return csr_read(CSR_TIME);
}

#endif /* ARM_TIMER_H */

