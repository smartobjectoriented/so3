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
#include <timer.h>
#include <delay.h>

#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/vexpress.h>

#include <device/arch/gic.h>

extern void secondary_startup(void);
extern void vexpress_secondary_startup(void);

static DEFINE_SPINLOCK(boot_lock);

/*
 * Called from CPU #0
 */
void smp_boot_secondary(unsigned int cpu) {
	unsigned int sysreg_base;

	sysreg_base = (unsigned int) io_map(VEXPRESS_SYSREG_BASE, VEXPRESS_SYSREG_SIZE);
	if (!sysreg_base) {
		printk("!!!! BOOTUP jump vectors can't be used !!!!\n");
		BUG();
	}

	iowrite32(sysreg_base + SYS_FLAGSCLR, ~0);
	iowrite32(sysreg_base + SYS_FLAGSSET, __pa(vexpress_secondary_startup));

	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * This is really belt and braces; we hold unintended secondary
	 * CPUs in the holding pen until we're ready for them.  However,
	 * since we haven't sent them a soft interrupt, they shouldn't
	 * be there.
	 */
	write_pen_release(cpu);

	/*
	 * Send the secondary CPU a soft interrupt, thereby causing
	 * the boot monitor to read the system wide flags register,
	 * and branch to the address found there.
	 */

	smp_cross_call((long) (1 << cpu), IPI_WAKEUP);

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);
}
