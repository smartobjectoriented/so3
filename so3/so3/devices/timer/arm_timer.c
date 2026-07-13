/*
 * Copyright (C) 2014-2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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

#include <timer.h>
#include <softirq.h>
#include <schedule.h>
#include <heap.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>
#include <device/timer.h>

#include <device/arch/arm_timer.h>

#include <asm/arm_timer.h>
#include <asm/processor.h>
#include <memory.h>

#ifdef CONFIG_AVZ
#include <avz/physdev.h>
#include <avz/memslot.h>
#endif

static void next_event(u32 next)
{
	unsigned long ctrl;

#ifdef CONFIG_AVZ
	ctrl = arch_timer_reg_read_el2(ARCH_TIMER_REG_CTRL);
#else
	ctrl = arch_timer_reg_read_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL);
#endif

	ctrl |= ARCH_TIMER_CTRL_ENABLE;
	ctrl &= ~ARCH_TIMER_CTRL_IT_MASK;

#ifdef CONFIG_AVZ
	arch_timer_reg_write_el2(ARCH_TIMER_REG_TVAL, next);
	arch_timer_reg_write_el2(ARCH_TIMER_REG_CTRL, ctrl);
#else
	arch_timer_reg_write_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_TVAL, next);
	arch_timer_reg_write_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL, ctrl);
#endif
}

static irq_return_t timer_isr(int irq, void *dev)
{
	unsigned long ctrl;
	arm_timer_t *arm_timer;

	arm_timer = (arm_timer_t *) dev_get_drvdata((dev_t *) dev);

	/* Clear the interrupt */

#ifdef CONFIG_AVZ
	ctrl = arch_timer_reg_read_el2(ARCH_TIMER_REG_CTRL);
#else
	ctrl = arch_timer_reg_read_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL);
#endif

	if (ctrl & ARCH_TIMER_CTRL_IT_STAT) {
		ctrl |= ARCH_TIMER_CTRL_IT_MASK;

#ifdef CONFIG_AVZ
		arch_timer_reg_write_el2(ARCH_TIMER_REG_CTRL, ctrl);
#else
		arch_timer_reg_write_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL, ctrl);
#endif

		/* Periodic timer */
		next_event(arm_timer->reload);

#ifdef CONFIG_AVZ
		/* TEMP (rpi4 bring-up diagnostics): sample the interrupted PC on
		 * the first EL2 ticks of the agency CPU. When the guest runs but
		 * stays silent, ELR_EL2 pinpoints where it is spinning (symbolize
		 * against the guest vmlinux). To be removed once the rpi4_64
		 * agency boots to the console. */
		{
			static volatile int elr_samples = 0;

			if ((smp_processor_id() == AGENCY_CPU) && (elr_samples < 8)) {
				elr_samples++;
				printk("EL2 tick #%d (CPU %d): ELR_EL2=0x%lx SPSR_EL2=0x%lx\n", elr_samples,
				       smp_processor_id(), read_sysreg(elr_el2), read_sysreg(spsr_el2));
			}
		}

		timer_interrupt((smp_processor_id() == S3C_CPU) ? true : false);
#else
		jiffies++;

		raise_softirq(TIMER_SOFTIRQ);
#endif
	}

	return IRQ_COMPLETED;
}

void periodic_timer_start(void)
{
	arm_timer_t *arm_timer = (arm_timer_t *) dev_get_drvdata(periodic_timer.dev);

	/* Start the periodic timer */
	next_event(arm_timer->reload);
}

#ifdef CONFIG_AVZ

/* Called from the EL2 IRQ handler when CNTHP (PPI 26) fires while Linux
 * runs at EL1.  Used by both GIC versions: the dispatch in gic.c
 * special-cases INTID 26 and invokes this directly, bypassing the
 * irq_desc action table.  This guarantees CNTHP gets re-armed even if a
 * spurious early CNTHP IRQ arrives before periodic_timer_init binds the
 * action — which would otherwise route the IRQ to the guest and leave
 * the timer one-shot.  Safe to call before periodic_timer.dev is set:
 * dev_get_drvdata returns NULL and we skip. 
 */
void avz_el2_timer_tick(void)
{
	arm_timer_t *arm_timer;

	if (!periodic_timer.dev)
		return;

	arm_timer = (arm_timer_t *) dev_get_drvdata(periodic_timer.dev);
	if (!arm_timer)
		return;

	/* Re-arm the timer for the next period. */
	next_event(arm_timer->reload);

	/* TEMP (rpi4 bring-up diagnostics): sample the interrupted PC on the
	 * first EL2 ticks of the agency CPU. This is the path actually taken
	 * for CNTHP (the INTID-26 special case bypasses timer_isr, where a
	 * first sampler sat and never fired). SPSR_EL2.M = 0x5 (EL1h) means
	 * the sample is the silent guest's PC — symbolize against vmlinux.
	 * To be removed once the rpi4_64 agency boots to the console. */
	{
		static volatile int elr_samples = 0;
		unsigned long spsr = read_sysreg(spsr_el2);

		/* Only sample interrupted EL1 contexts (SPSR_EL2.M[3:2] = 01,
		 * i.e. the guest): that is the PC we are after, and it keeps
		 * this printk out of any window where AVZ itself is mid-print
		 * (a tick landing during a boot-time printk deadlocked on the
		 * console lock when this sampled unconditionally). */
		if ((smp_processor_id() == AGENCY_CPU) && (elr_samples < 12) && ((spsr & 0xc) == 0x4)) {
			elr_samples++;
			/* The guest is looping through its own exception handlers:
			 * its EL1 exception registers (readable from EL2 while the
			 * vcpu is current) carry the ORIGINAL fault: ELR_EL1 = the
			 * faulting guest PC, ESR_EL1 = the cause. */
			printk("EL2 tick #%d: guest ELR_EL2=0x%lx SPSR_EL2=0x%lx | ELR_EL1=0x%lx ESR_EL1=0x%lx FAR_EL1=0x%lx\n",
			       elr_samples, read_sysreg(elr_el2), spsr, read_sysreg(elr_el1), read_sysreg(esr_el1),
			       read_sysreg(far_el1));

			if (elr_samples == 12) {
				/* TEMP: probe the guest text at the faulting PC. The
				 * guest undef'd on what the vmlinux says is a NOP, so
				 * either the RAM really is corrupted or the guest's
				 * view (icache/S2) is stale. Read the physical bytes
				 * through the agency memslot mapping. */
				unsigned long elr1 = read_sysreg(elr_el1);

				if ((elr1 & 0xffffffc000000000UL) == 0xffffffc000000000UL) {
					unsigned long pa = elr1 - 0xffffffc080000000UL + 0x1000000UL;
					u32 *txt = (u32 *) __xva(MEMSLOT_AGENCY, pa);

					printk("guest text @ELR_EL1 0x%lx (PA 0x%lx): %08x %08x %08x %08x\n", elr1, pa,
					       txt[0], txt[1], txt[2], txt[3]);
				}

				/* Fixed corruption-watch window (same as the
				 * pre-unpause dump in setup.c). */
				{
					u32 *g = (u32 *) __xva(MEMSLOT_AGENCY, 0x11f2f40UL);

					printk("guest text @tick12 @PA 0x11f2f40:\n");
					printk("  %08x %08x %08x %08x %08x %08x %08x %08x\n", g[0], g[1], g[2], g[3],
					       g[4], g[5], g[6], g[7]);
					printk("  %08x %08x %08x %08x %08x %08x %08x %08x\n", g[8], g[9], g[10], g[11],
					       g[12], g[13], g[14], g[15]);
				}

				/* TEMP: the guest BUGs before its console is up, so
				 * its whole early dmesg (incl. the original panic and
				 * stack trace) sits unseen in the printk ring. Dump it
				 * from EL2 through the agency memslot mapping. PA =
				 * __log_buf guest VA (0xffffffc0819b1330 for THIS
				 * vmlinux) - kernel VA base + guest load PA 0x1000000
				 * = 0x29b1330. */
				u8 *lb = (u8 *) __xva(MEMSLOT_AGENCY, 0x29b1330UL);
				static char line[121];
				int i, n = 0;

				printk("==== guest __log_buf (16 KB) ====\n");
				for (i = 0; i < 16384; i++) {
					u8 c = lb[i];

					if (c >= 0x20 && c < 0x7f) {
						line[n++] = c;
						if (n == 120) {
							line[n] = 0;
							printk("%s\n", line);
							n = 0;
						}
					} else if ((c == '\n') && n) {
						line[n] = 0;
						printk("%s\n", line);
						n = 0;
					}
				}
				if (n) {
					line[n] = 0;
					printk("%s\n", line);
				}
				printk("==== end log_buf ====\n");
			}
		}
	}

	/* Same CPU predicate as arm_timer_isr: on the capsule CPU the tick
	 * must run the periodic path so capsule domains get their
	 * VIRQ_TIMER event; otherwise a capsule never sees a tick and
	 * spins forever in calibrate_delay. */
	timer_interrupt(smp_processor_id() == S3C_CPU);
}
#endif /* CONFIG_AVZ */

/*
 * Read the clocksource timer value taking into account a time reference.
 *
 */
u64 clocksource_read(void)
{
	return arch_counter_get_cntvct();
}

void secondary_timer_init(void)
{
	arm_timer_t *arm_timer = (arm_timer_t *) dev_get_drvdata(periodic_timer.dev);

#ifndef CONFIG_AVZ
	unsigned long ctrl;
#endif

	/* Shutdown the timer */

#ifdef CONFIG_AVZ
	arch_timer_reg_write_el2(ARCH_TIMER_REG_CTRL, 0);
#else
	ctrl = arch_timer_reg_read_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL);
	ctrl &= ~ARCH_TIMER_CTRL_ENABLE;
	arch_timer_reg_write_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL, ctrl);
#endif

	/* Bind ISR into interrupt controller */
	irq_unmask(arm_timer->irq_def.irqnr);
}

/*
 * Initialize the periodic timer used by the kernel.
 */
static int periodic_timer_init(dev_t *dev, int fdt_offset)
{
#ifndef CONFIG_AVZ
	unsigned long ctrl;
#endif
	arm_timer_t *arm_timer;

	periodic_timer.dev = dev;

	/* Pins multiplexing skipped here for simplicity (done by bootloader) */
	/* Clocks init skipped here for simplicity (done by bootloader) */

	arm_timer = (arm_timer_t *) malloc(sizeof(arm_timer_t));
	BUG_ON(!arm_timer);

	fdt_interrupt_node(fdt_offset, &arm_timer->irq_def);

	/* Pins multiplexing skipped here for simplicity (done by bootloader) */
	/* Clocks init skipped here for simplicity (done by bootloader) */

	/* Initialize Timer */

	periodic_timer.start = periodic_timer_start;
	periodic_timer.period = NSECS / CONFIG_HZ;

	arm_timer->reload = (uint32_t) (periodic_timer.period / (NSECS / clocksource_timer.rate));

	/* Shutdown the timer */

#ifdef CONFIG_AVZ
	arch_timer_reg_write_el2(ARCH_TIMER_REG_CTRL, 0);
#else
	ctrl = arch_timer_reg_read_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL);
	ctrl &= ~ARCH_TIMER_CTRL_ENABLE;
	arch_timer_reg_write_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL, ctrl);
#endif

	dev_set_drvdata(dev, arm_timer);

	/* Bind ISR into interrupt controller */
	irq_bind(arm_timer->irq_def.irqnr, timer_isr, NULL, dev);

	return 0;
}

/*
 * Initialize the clocksource timer for free-running timer (used for system time)
 */
static int clocksource_timer_init(dev_t *dev, int fdt_offset)
{
	clocksource_timer.cycle_last = 0;

	clocksource_timer.read = clocksource_read;
	clocksource_timer.rate = arch_timer_get_cntfrq();
	clocksource_timer.mask = CLOCKSOURCE_MASK(56);

	/* Compute the various parameters for this clocksource */
	clocks_calc_mult_shift(&clocksource_timer.mult, &clocksource_timer.shift, clocksource_timer.rate, NSECS, 3600);

	return 0;
}

REGISTER_DRIVER_CORE("arm,clocksource-timer", clocksource_timer_init);

/* Need the clocksource rate to initialize the periodic timer. */
REGISTER_DRIVER_POSTCORE("arm,periodic-timer", periodic_timer_init);
