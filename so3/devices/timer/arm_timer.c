/*
 * Copyright (C) 2014-2022 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

static void next_event(u32 next) {

	unsigned long ctrl;

#ifdef CONFIG_ARM64VT
	ctrl = arch_timer_reg_read_el2(ARCH_TIMER_REG_CTRL);
#else
	ctrl = arch_timer_reg_read_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL);
#endif

	ctrl |= ARCH_TIMER_CTRL_ENABLE;
	ctrl &= ~ARCH_TIMER_CTRL_IT_MASK;

#ifdef CONFIG_ARM64VT
	arch_timer_reg_write_el2(ARCH_TIMER_REG_TVAL, next);
	arch_timer_reg_write_el2(ARCH_TIMER_REG_CTRL, ctrl);
#else
	arch_timer_reg_write_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_TVAL, next);
	arch_timer_reg_write_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL, ctrl);
#endif
}

static irq_return_t timer_isr(int irq, void *dev) {
	unsigned long ctrl;
	arm_timer_t *arm_timer;

	arm_timer = (arm_timer_t *) dev_get_drvdata((dev_t *) dev);

	/* Clear the interrupt */

#ifdef CONFIG_ARM64VT
	ctrl = arch_timer_reg_read_el2(ARCH_TIMER_REG_CTRL);
#else
	ctrl = arch_timer_reg_read_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL);
#endif

	if (ctrl & ARCH_TIMER_CTRL_IT_STAT) {
		ctrl |= ARCH_TIMER_CTRL_IT_MASK;

#ifdef CONFIG_ARM64VT
		arch_timer_reg_write_el2(ARCH_TIMER_REG_CTRL, ctrl);
#else
		arch_timer_reg_write_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL, ctrl);
#endif

		/* Periodic timer */
		next_event(arm_timer->reload);

#ifdef CONFIG_AVZ
		timer_interrupt((smp_processor_id() == ME_CPU) ? true : false);
#else
		jiffies++;

		raise_softirq(TIMER_SOFTIRQ);
#endif
	}

	return IRQ_COMPLETED;
}

void periodic_timer_start(void) {
	arm_timer_t *arm_timer = (arm_timer_t *) dev_get_drvdata(periodic_timer.dev);

	/* Start the periodic timer */
	next_event(arm_timer->reload);
}

/*
 * Read the clocksource timer value taking into account a time reference.
 *
 */
u64 clocksource_read(void) {
	return arch_counter_get_cntvct();
}

void secondary_timer_init(void) {
	arm_timer_t *arm_timer = (arm_timer_t *) dev_get_drvdata(periodic_timer.dev);

#ifndef CONFIG_ARM64VT
	unsigned long ctrl;
#endif

	/* Shutdown the timer */

#ifdef CONFIG_ARM64VT
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
static int periodic_timer_init(dev_t *dev, int fdt_offset) {
#ifndef CONFIG_ARM64VT
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

#ifdef CONFIG_ARM64VT
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
static int clocksource_timer_init(dev_t *dev, int fdt_offset) {

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

