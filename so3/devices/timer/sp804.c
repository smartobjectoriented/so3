/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

/*
 *
 * Simple TIMER handling routines for Allwinner A20 chip
 * For now this implementation only supports TIMER0. 
 * Reference: Allwinner A20 User Manual: Section 1.9 Timer
 */

#include <common.h>
#include <softirq.h>
#include <schedule.h>
#include <heap.h>
#include <timer.h>
#include <memory.h>

#include <device/timer.h>
#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>

#include <device/arch/sp804.h>

#include <asm/io.h>

typedef struct {
	struct sp804_timer *base;
	irq_def_t irq_def;
} sp804_t;

static irq_return_t timer_isr(int irq, void *dev) {
	sp804_t *sp804 = (sp804_t *) dev_get_drvdata((dev_t *) dev);

	/* Clear the interrupt */
	iowrite32(&sp804->base->timerintclr, 1);

	jiffies++;

	raise_softirq(TIMER_SOFTIRQ);

	return IRQ_COMPLETED;
}

#if 0
static void next_event(u32 next) {

	unsigned long ctrl = ioread32(oneshot_timer.dev->base + TIMER_CTRL);

	iowrite32(oneshot_timer.dev->base + TIMER_LOAD, next);

	iowrite32(oneshot_timer.dev->base + TIMER_CTRL, ctrl | TIMER_CTRL_ENABLE);
}
#endif


static void periodic_timer_start(void) {
	uint32_t ctrl = TIMER_CTRL_32BIT | TIMER_CTRL_IE;
	uint32_t reload;
	sp804_t *sp804 = (sp804_t *) dev_get_drvdata((dev_t *) periodic_timer.dev);

	/* Number of clock cycles to wait */
	reload = (uint32_t) (periodic_timer.period / (NSECS / TIMER_RATE));

	iowrite32(&sp804->base->timercontrol, ctrl);

	printk("%s: setting up system timer periodic freq at %x\n", __func__, reload);

	iowrite32(&sp804->base->timerload, reload);
	ctrl |= TIMER_CTRL_PERIODIC | TIMER_CTRL_ENABLE;

	iowrite32(&sp804->base->timercontrol, ctrl);

	/* Enable interrupt (IRQ controller) */
	irq_ops.enable(sp804->irq_def.irqnr);
}

static void periodic_timer_stop(void) {
	uint32_t ctrl = ~TIMER_CTRL_ENABLE;
	sp804_t *sp804 = (sp804_t *) dev_get_drvdata((dev_t *) periodic_timer.dev);

	iowrite32(&sp804->base->timercontrol, ctrl);
}

/* Clocksource */

/* Read the clocksource timer value */
u64 clocksource_read(void) {
	sp804_t *sp804 = (sp804_t *) dev_get_drvdata((dev_t *) clocksource_timer.dev);

	return (u64) ~ioread32(&sp804->base->timervalue);
}

/*
 * Initialize the periodic timer used by the kernel.
 */
static int periodic_timer_init(dev_t *dev, int fdt_offset) {
	const struct fdt_property *prop;
	int prop_len;
	sp804_t *sp804;

	periodic_timer.dev = dev;

	/* Pins multiplexing skipped here for simplicity (done by bootloader) */
	/* Clocks init skipped here for simplicity (done by bootloader) */

	sp804 = malloc(sizeof(sp804_t));
	BUG_ON(!sp804);

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);

	BUG_ON(prop_len != 2 * sizeof(unsigned long));

	sp804->base = (struct sp804_timer *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]), fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));

	fdt_interrupt_node(fdt_offset, &sp804->irq_def);

	/* Initialize Timer */

	periodic_timer.start = periodic_timer_start;
	periodic_timer.stop = periodic_timer_stop;
	periodic_timer.period = NSECS / CONFIG_HZ;

	dev_set_drvdata(dev, sp804);

	/* Bind ISR into interrupt controller */
	irq_bind(sp804->irq_def.irqnr, timer_isr, NULL, dev);

	return 0;
}

#if 0 /* To be completed further... */
static int oneshot_timer_set_delay(uint64_t delay_ns) {

	delta = min((u64 ) delta, oneshot_timer.max_delta_ns);
	delta = max((u64 ) delta, oneshot_timer.min_delta_ns);

	clc = (delta * oneshot_timer.mult) >> oneshot_timer.shift;
	oneshot_timer.next_event((unsigned long) clc);

}

static int oneshot_timer_init(dev_t *dev) {


}
#endif

/**
 * Initialize the clocksource timer for free-running timer (used for system time)
 *
 * @param dev
 * @param fdt_offset Location of the timer node
 * @return
 */
static int clocksource_timer_init(dev_t *dev, int fdt_offset) {
	const struct fdt_property *prop;
	int prop_len;
	sp804_t *sp804;

	clocksource_timer.dev = dev;

	sp804 = malloc(sizeof(sp804_t));
	BUG_ON(!sp804);

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);

	BUG_ON(prop_len != 2 * sizeof(unsigned long));

	sp804->base = (struct sp804_timer *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]), fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));

	clocksource_timer.cycle_last = 0;

	clocksource_timer.mask = CLOCKSOURCE_MASK(32);
	clocksource_timer.rate = TIMER_RATE;

	clocksource_timer.read = clocksource_read;

	/* This free-running counter is used only for the associated HS (RT) timer to sync */
	/* setup timer 2 as free-running clocksource */

	iowrite32(&sp804->base->timercontrol, 0);
	iowrite32(&sp804->base->timerload, 0xffffffff);
	iowrite32(&sp804->base->timervalue, 0xffffffff);

	iowrite32(&sp804->base->timercontrol, TIMER_CTRL_32BIT | TIMER_CTRL_ENABLE | TIMER_CTRL_PERIODIC);

	/* Calculate the mult/shift to convert counter ticks to ns. */
	clocks_calc_mult_shift(&clocksource_timer.mult, &clocksource_timer.shift, clocksource_timer.rate, NSECS, 3600);

	dev_set_drvdata(dev, sp804);

	return 0;
}

REGISTER_DRIVER_CORE("sp804,periodic-timer", periodic_timer_init);
REGISTER_DRIVER_CORE("sp804,clocksource-timer", clocksource_timer_init);

