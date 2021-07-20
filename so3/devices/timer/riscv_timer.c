/*
 * Copyright (C) 2021 Nicolas Müller <nicolas.muller1@heig-vd.ch>
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

#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>
#include <device/timer.h>

#include <device/arch/riscv_timer.h>
#include <mach/timer.h>

static unsigned long reload;

static void next_event(u32 next) {

	u64 *mtimecmp_addr = (u64*) TIMER_MTIMECMP_REG;

	/* Enables IRQs from timer */
	csr_set(CSR_IE, IE_TIE);

	/* Set new CMP register value. This clears interrupt as well. Interrupt is only the
	 * result of the comparator between mtime and mtimecmp. If correct value is written,
	 * interrupt is cleared */
	*mtimecmp_addr = arch_get_time() + next;
}

static irq_return_t timer_isr(int irq, void *dummy) {

#if 0 /* _NMR_ no irqs yet */
	unsigned long ctrl;

	/* Clear the interrupt */

	ctrl = arch_timer_reg_read_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL);

	if (ctrl & ARCH_TIMER_CTRL_IT_STAT) {
		ctrl |= ARCH_TIMER_CTRL_IT_MASK;
		arch_timer_reg_write_cp15(ARCH_TIMER_VIRT_ACCESS, ARCH_TIMER_REG_CTRL, ctrl);

		/* Periodic timer. Reloading here will clear the interrupt */
		next_event(reload);

		jiffies++;

		raise_softirq(TIMER_SOFTIRQ);
	}
#endif

	return IRQ_COMPLETED;
}

static void periodic_timer_start(void) {
	/* Start the periodic timer */
	next_event(reload);
}

/*
 * Read the clocksource timer value taking into account a time reference.
 *
 */
u64 clocksource_read(void) {
	return arch_get_time();
}

/*
 * Initialize the periodic timer used by the kernel.
 */
static int periodic_timer_init(dev_t *dev) {

	periodic_timer.dev = dev;

	/* Pins multiplexing skipped here for simplicity (done by bootloader) */
	/* Clocks init skipped here for simplicity (done by bootloader) */

	/* Initialize Timer */

	periodic_timer.start = periodic_timer_start;
	periodic_timer.period = NSECS / HZ;

	reload = (uint32_t) (periodic_timer.period / (NSECS / clocksource_timer.rate));

#if 0 /* _NMR_ no irq yet*/
	/* Bind ISR into interrupt controller */
	irq_bind(dev->irq_nr, timer_isr, NULL, NULL);
#endif

	/* Disable the timer interrupts */
	csr_clear(CSR_IE, IE_TIE);

	return 0;
}

/*
 * Initialize the clocksource timer for free-running timer (used for system time)
 */
static int clocksource_timer_init(dev_t *dev) {

	clocksource_timer.dev = dev;
	clocksource_timer.cycle_last = 0;

	clocksource_timer.read = clocksource_read;
	clocksource_timer.rate = arch_timer_get_cntfrq();
	clocksource_timer.mask = CLOCKSOURCE_MASK(56);

	/* Compute the various parameters for this clocksource */
	clocks_calc_mult_shift(&clocksource_timer.mult, &clocksource_timer.shift, clocksource_timer.rate, NSECS, 3600);

	return 0;
}

REGISTER_DRIVER_CORE("riscv,clocksource-timer", clocksource_timer_init);

/* Need the clocksource rate to initialize the periodic timer. */
REGISTER_DRIVER_POSTCORE("riscv,periodic-timer", periodic_timer_init);

