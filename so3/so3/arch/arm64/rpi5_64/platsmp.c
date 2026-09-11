/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <timer.h>

#include <asm/processor.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <device/arch/gic.h>

extern void secondary_startup(void);

static DEFINE_SPINLOCK(cpu_lock);

/*
 * The BCM2712 brings its secondary cores up through PSCI -- the Linux
 * device tree says enable-method = "psci" for all four -- so this is
 * virt64's bring-up, not rpi4's. The Pi 4 had no PSCI: its cores spun in
 * the armstub spin table and AVZ had to emulate the release protocol,
 * which is where arch/arm64/rpi4_64 gets its mailbox and CPU?_RELEASE_ADDR
 * definitions from. None of that is needed here.
 */

void smp_boot_secondary(unsigned int cpu)
{
	spin_lock(&cpu_lock);

	cpu_on(cpu, (addr_t) __pa(secondary_startup));

	spin_unlock(&cpu_lock);
}
