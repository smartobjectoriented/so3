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
 * Reference: Allwinner A53 User Manual: Section 3.6.4 Timer
 */

#include <softirq.h>
#include <schedule.h>
#include <timer.h>

#include <device/timer.h>
#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>

#include <mach/timer.h>

#include <device/arch/sun4i_timer.h>

#include <asm/io.h>

static irq_return_t timer_isr(int irq, void *dummy) {
	sun4i_timer_t *sun4i_timer = (sun4i_timer_t *) periodic_timer.dev->base;
	unsigned int timer_sta;

	/* clear the interrupt */
	timer_sta = ioread32(&sun4i_timer->timer_status);
	timer_sta |= TIMER_IRQ_ST_T0;
	iowrite32(&sun4i_timer->timer_status, timer_sta);

	/* Process the timer interrupt */
	jiffies++;

	raise_softirq(TIMER_SOFTIRQ);

	return IRQ_COMPLETED;
}

/** @brief Currently this function is taking care of only timer 1
 *  TODO : Create a way of starting not only one, but multiple timers
 */
static int timer_start(int delay_ns, int periodic) {
	sun4i_timer_t *sun4i_timer = (sun4i_timer_t *) periodic_timer.dev->base;
	unsigned long ctrl;

	/* Source OSC is 24 MHz and there is no prescaler. */

	iowrite32(&sun4i_timer->timers[TIMER_SYS_TIMER].timerintvalue, delay_ns / (NSECS / TIMER_RATE));

	ctrl = ioread32(&sun4i_timer->timers[TIMER_SYS_TIMER].timercontrol);

	if (!periodic)
		/* period set, and timer enabled in 'next_event' hook */
		ctrl |= TIMER_CTRL_ONESHOT;

	/* Enable interrupt (IRQ controller) */
	irq_ops.irq_enable(periodic_timer.dev->irq);

	/* Re-init the timer */
	ctrl |= TIMER_CTRL_RELOAD;
	iowrite32(&sun4i_timer->timers[TIMER_SYS_TIMER].timercontrol, ctrl);

	/* Waiting the reload bit is reset */
	while ((ioread32(&sun4i_timer->timers[TIMER_SYS_TIMER].timercontrol) >> 1) & 1) ;

	iowrite32(&sun4i_timer->timers[TIMER_SYS_TIMER].timercontrol,
			ioread32(&sun4i_timer->timers[TIMER_SYS_TIMER].timercontrol) | TIMER_CTRL_ENABLE);

	return 0;
}

static void periodic_timer_start(void) {

	/* Start timer in periodic mode */
	timer_start(periodic_timer.period, true);
}

#if 0 /* not used */

static int oneshot_timer_start(int delay_us) {

	/* Start timer in oneshot mode */
	return timer_start(delay_us, false);
}

/**
 * @brief This function stops the timer > 0.
 */
static int timer_stop(void) {
	sun4i_timer_t *sun4i_timer = (sun4i_timer_t *) periodic_timer.dev->base;
	unsigned int timer_reg;

	timer_reg = ioread32(&sun4i_timer->timers[TIMER_SYS_TIMER].timercontrol);
	timer_reg &= TIMER_CTRL_DISABLE;
	iowrite32(&sun4i_timer->timers[TIMER_SYS_TIMER].timercontrol, timer_reg);

	return 0;
}
#endif /* 0 */

static int sun4i_timer_init(dev_t *dev)
{
	sun4i_timer_t *sun4i_timer;
	unsigned long ctrl;

	periodic_timer.dev = dev;
	sun4i_timer = (sun4i_timer_t *) periodic_timer.dev->base;

	/* Pins multiplexing skipped here for simplicity (done by bootloader) */
	/* Clocks init skipped here for simplicity (done by bootloader) */

	/* Set-up the timer IRQ */
	iowrite32(&sun4i_timer->timer_irqen, TIMER_IRQ_EN_T0 | TIMER_IRQ_EN_T1);
	iowrite32(&sun4i_timer->timer_status, TIMER_IRQ_CLR);

	ctrl = ioread32(&sun4i_timer->timers[TIMER_SYS_TIMER].timercontrol);

	ctrl |= TIMER_CTRL_DIV1 | TIMER_CTRL_CLK_OSC24M;

	iowrite32(&sun4i_timer->timers[TIMER_SYS_TIMER].timercontrol, ctrl);

	periodic_timer.start = periodic_timer_start;
	periodic_timer.period = NSECS / HZ;

	/* Bind ISR into interrupt controller */
	irq_bind(dev->irq, timer_isr, NULL, NULL);

	return 0;
}

REGISTER_DRIVER_CORE("sun4i-timer,periodic-timer", sun4i_timer_init);


