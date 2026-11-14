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
/*
 * PC timers: the 8254 PIT channel 0 gives the periodic tick on IRQ 0, and
 * the TSC is the clocksource, calibrated against the PIT channel 2.
 */

#include <timer.h>
#include <softirq.h>
#include <schedule.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>
#include <device/timer.h>

#include <asm/io.h>
#include <asm/processor.h>

#define PIT_FREQ 1193182

#define PIT_CH0 0x40
#define PIT_CH2 0x42
#define PIT_CMD 0x43
#define PIT_GATE 0x61

#define PIT_GATE_CH2 0x01
#define PIT_SPEAKER 0x02
#define PIT_OUT_CH2 0x20

/* Calibration window: 1/CALIBRATE_DIV second */

#define CALIBRATE_DIV 20

static irq_def_t pit_irq;

/* The TSC is scaled down so that its rate fits the 32-bit clocksource rate */

static unsigned int tsc_shift;

static irq_return_t pit_isr(int irq, void *dev)
{
	jiffies++;
	raise_softirq(TIMER_SOFTIRQ);

	return IRQ_COMPLETED;
}

static void pit_start(void)
{
	u16 divisor = PIT_FREQ / CONFIG_HZ;

	/* Channel 0, lobyte/hibyte, mode 2 (rate generator) */

	outb(PIT_CMD, 0x34);
	outb(PIT_CH0, divisor & 0xff);
	outb(PIT_CH0, divisor >> 8);
}

static int pit_init(dev_t *dev, int fdt_offset)
{
	periodic_timer.dev = dev;

	fdt_interrupt_node(fdt_offset, &pit_irq);

	periodic_timer.start = pit_start;
	periodic_timer.period = NSECS / CONFIG_HZ;

	irq_bind(pit_irq.irqnr, pit_isr, NULL, dev);

	return 0;
}

static u64 tsc_read(void)
{
	return rdtsc() >> tsc_shift;
}

/* Count the TSC cycles while the PIT channel 2 counts down a known delay */

static u64 tsc_calibrate(void)
{
	u16 latch = PIT_FREQ / CALIBRATE_DIV;
	u64 t0, t1;

	outb(PIT_GATE, (inb(PIT_GATE) & ~PIT_SPEAKER) | PIT_GATE_CH2);

	/* Channel 2, lobyte/hibyte, mode 0 (interrupt on terminal count) */

	outb(PIT_CMD, 0xb0);
	outb(PIT_CH2, latch & 0xff);
	outb(PIT_CH2, latch >> 8);

	t0 = rdtsc();
	while (!(inb(PIT_GATE) & PIT_OUT_CH2))
		;
	t1 = rdtsc();

	return (t1 - t0) * CALIBRATE_DIV;
}

static int tsc_init(dev_t *dev, int fdt_offset)
{
	u64 tsc_hz = tsc_calibrate();

	tsc_shift = 0;
	while ((tsc_hz >> tsc_shift) > 0x7fffffffULL)
		tsc_shift++;

	clocksource_timer.cycle_last = 0;
	clocksource_timer.read = tsc_read;
	clocksource_timer.rate = tsc_hz >> tsc_shift;
	clocksource_timer.mask = CLOCKSOURCE_MASK((64 - tsc_shift));

	clocks_calc_mult_shift(&clocksource_timer.mult, &clocksource_timer.shift, clocksource_timer.rate, NSECS, 3600);

	printk("TSC clocksource: %lu kHz\n", (unsigned long) (tsc_hz / 1000));

	return 0;
}

REGISTER_DRIVER_CORE("x86,tsc", tsc_init);
REGISTER_DRIVER_POSTCORE("intel,8254", pit_init);
