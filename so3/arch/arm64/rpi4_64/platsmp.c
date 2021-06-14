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
#include <io.h>
#include <spinlock.h>
#include <lib.h>
#include <memory.h>

#include <asm/cacheflush.h>
#include <asm/processor.h>

#include <device/arch/gic.h>

#include <mach/rpi4.h>

extern void secondary_startup(void);


void smp_prepare_cpus(unsigned int max_cpus)
{
	/* Nothing to do for Rpi4 */
}

void smp_boot_secondary(unsigned int cpu)
{
	unsigned long secondary_startup_phys = (unsigned long) virt_to_phys((void *) secondary_startup);
	void *intc_vaddr;

	printk("%s: booting CPU: %d...\n", __func__, cpu);

	intc_vaddr = ioremap(LOCAL_INTC_PHYS, LOCAL_INTC_SIZE);

	writel(secondary_startup_phys, intc_vaddr + LOCAL_MAILBOX3_SET0 + 16 * cpu);

	dsb(sy);
	sev();
}

void smp_secondary_init(unsigned int cpu) {
	/* Nothing to do for Rpi4 */
}

