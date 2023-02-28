/*
 * Copyright (C) 2014-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef SOFTIRQ_H
#define SOFTIRQ_H

/* Low-latency softirqs come first in the following list.
 * SCHEDULE_SOFTIRQ must remain the last in the list.
 */
enum {
	TIMER_SOFTIRQ = 0,
	SCHEDULE_SOFTIRQ,
	NR_COMMON_SOFTIRQS
};

#include <common.h>
#include <smp.h>

#define NR_SOFTIRQS NR_COMMON_SOFTIRQS

typedef void (*softirq_handler)(void);

void register_softirq(int nr, softirq_handler handler);
void raise_softirq(unsigned int nr);
void softirq_init(void);
void do_softirq(void);

void cpu_raise_softirq(unsigned int cpu, unsigned int nr);
void raise_softirq(unsigned int nr);

#endif /* SOFTIRQ_H */
