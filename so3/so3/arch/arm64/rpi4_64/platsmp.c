/*
 * Copyright (C) 2016,2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <smp.h>
#include <spinlock.h>
#include <memory.h>

#include <asm/cacheflush.h>
#include <asm/processor.h>

#include <device/arch/gic.h>

#include <mach/io.h>

extern void secondary_startup(void);

static DEFINE_SPINLOCK(cpu_lock);

void smp_boot_secondary(unsigned int cpu)
{
	printk("%s: booting CPU: %d\n", __func__, cpu);

	spin_lock(&cpu_lock);
	cpu_on(cpu, (u32) __pa(secondary_startup));
	spin_unlock(&cpu_lock);
}
