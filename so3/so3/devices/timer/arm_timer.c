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

#ifdef CONFIG_AVZ
#include <avz/physdev.h>
#endif

/*
 * The periodic timer is the EL2 timer under AVZ, the virtual timer otherwise.
 */

static inline u32 timer_read(enum arch_timer_reg reg)
{
#ifdef CONFIG_AVZ
	return arch_timer_reg_read_el2(reg);
#else
	return arch_timer_reg_read_cp15(ARCH_TIMER_VIRT_ACCESS, reg);
#endif
}

static inline void timer_write(enum arch_timer_reg reg, u32 val)
{
#ifdef CONFIG_AVZ
	arch_timer_reg_write_el2(reg, val);
#else
	arch_timer_reg_write_cp15(ARCH_TIMER_VIRT_ACCESS, reg, val);
#endif
}

static inline u64 timer_get_cval(void)
{
#ifdef CONFIG_AVZ
	return arch_timer_get_cval_el2();
#else
	return arch_timer_get_cval_cp15(ARCH_TIMER_VIRT_ACCESS);
#endif
}

static inline void timer_set_cval(u64 cval)
{
#ifdef CONFIG_AVZ
	arch_timer_set_cval_el2(cval);
#else
	arch_timer_set_cval_cp15(ARCH_TIMER_VIRT_ACCESS, cval);
#endif
}

/*
 * Move the deadline one period after the previous one, not after now:
 * the IRQ latency then no longer accumulates into the tick. After a
 * stretch longer than a period with IRQs off, restart from now instead
 * of replaying every missed tick.
 */

static void next_period(u32 period)
{
	u64 cval = timer_get_cval() + period;
	u64 now = arch_counter_get_cntvct();

	if (cval <= now)
		cval = now + period;

	timer_set_cval(cval);
}

static void next_event(u32 next)
{
	u32 ctrl = timer_read(ARCH_TIMER_REG_CTRL);

	ctrl |= ARCH_TIMER_CTRL_ENABLE;
	ctrl &= ~ARCH_TIMER_CTRL_IT_MASK;

	timer_write(ARCH_TIMER_REG_TVAL, next);
	timer_write(ARCH_TIMER_REG_CTRL, ctrl);
}

static irq_return_t timer_isr(int irq, void *dev)
{
	arm_timer_t *arm_timer = (arm_timer_t *) dev_get_drvdata((dev_t *) dev);

	if (timer_read(ARCH_TIMER_REG_CTRL) & ARCH_TIMER_CTRL_IT_STAT) {
		/* Writing CVAL moves the deadline and clears the condition. */

		next_period(arm_timer->reload);

#if defined(CONFIG_AVZ) && defined(CONFIG_SOO)
		timer_interrupt(smp_processor_id() == S3C_CPU);
#elif defined(CONFIG_AVZ)
		/* No capsule CPU without CONFIG_SOO — always the agency path. */
		timer_interrupt(false);
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
	next_period(arm_timer->reload);

	/* Same CPU predicate as arm_timer_isr: on the capsule CPU the tick
	 * must run the periodic path so capsule domains get their
	 * VIRQ_TIMER event; otherwise a capsule never sees a tick and
	 * none of its timers ever fires. Without CONFIG_SOO there is
	 * no capsule CPU — every CPU runs the agency path. */

#ifdef CONFIG_SOO
	timer_interrupt(smp_processor_id() == S3C_CPU);
#else
	timer_interrupt(false);
#endif /* CONFIG_SOO */
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

	/* Shutdown the timer */

	timer_write(ARCH_TIMER_REG_CTRL, timer_read(ARCH_TIMER_REG_CTRL) & ~ARCH_TIMER_CTRL_ENABLE);

	/* Bind ISR into interrupt controller */
	irq_unmask(arm_timer->irq_def.irqnr);
}

/*
 * Initialize the periodic timer used by the kernel.
 */
static int periodic_timer_init(dev_t *dev, int fdt_offset)
{
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

	arm_timer->reload = clocksource_timer.rate / CONFIG_HZ;

	/* Shutdown the timer */

	timer_write(ARCH_TIMER_REG_CTRL, timer_read(ARCH_TIMER_REG_CTRL) & ~ARCH_TIMER_CTRL_ENABLE);

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
