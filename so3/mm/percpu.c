/*
 * Copyright (C) 2014-2016 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <common.h>
#include <memory.h>
#include <heap.h>
#include <errno.h>
#include <string.h>
#include <percpu.h>

extern char __per_cpu_start[], __per_cpu_data_end[], __per_cpu_end[];
unsigned long __per_cpu_offset[CONFIG_NR_CPUS];

#define INVALID_PERCPU_AREA (-(long)__per_cpu_start)
#define PERCPU_ORDER (get_order_from_bytes(__per_cpu_data_end - __per_cpu_start))

void percpu_init_areas(void)
{
    unsigned int cpu;

    for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++)
        __per_cpu_offset[cpu] = INVALID_PERCPU_AREA;
}

int init_percpu_area(unsigned int cpu)
{
    char *p;

    if (__per_cpu_offset[cpu] != INVALID_PERCPU_AREA)
        BUG();

    if ((p = memalign(PAGE_SIZE, PAGE_SIZE)) == NULL)
        BUG();

    memset(p, 0, __per_cpu_data_end - __per_cpu_start);
    __per_cpu_offset[cpu] = p - __per_cpu_start;

    return 0;
}




