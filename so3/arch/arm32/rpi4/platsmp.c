/*
 * Copyright (C) 2016,2017 Daniel Rossier <daniel.rossier@soo.tech>
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
#include <asm/io.h>

#include <device/arch/gic.h>

#include <mach/io.h>

extern void secondary_startup(void);

void smp_boot_secondary(unsigned int cpu)
{
	unsigned long secondary_startup_phys = (unsigned long) __pa((void *) secondary_startup);
	void *intc_vaddr; /* We will add bytes to this pointer */

	printk("%s: booting CPU: %d...\n", __func__, cpu);

	intc_vaddr = (void *) io_map(LOCAL_INTC_PHYS, LOCAL_INTC_SIZE);

	iowrite32(intc_vaddr + LOCAL_MAILBOX3_SET0 + 16 * cpu, secondary_startup_phys);

	dsb(sy);
	sev();
}


