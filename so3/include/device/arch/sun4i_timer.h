/*
 * Copyright (C) 2014-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef SUN4I_TIMER_H
#define SUN4I_TIMER_H

#include <types.h>

#define TIMER_RATE		24000000  /* This is the frequency of the timer [Hz] */

#define TIMER_CTRL_ENABLE       (1 << 0)
#define TIMER_CTRL_DISABLE      ~TIMER_CTRL_ENABLE
#define TIMER_CTRL_RELOAD		(1 << 1)
#define TIMER_CTRL_DIV1         (0 << 4)
#define TIMER_CTRL_DIV2         (1 << 4)
#define TIMER_CTRL_DIV4         (2 << 4)
#define TIMER_CTRL_DIV8         (3 << 4)
#define TIMER_CTRL_DIV16        (4 << 4)
#define TIMER_CTRL_DIV32        (5 << 4)
#define TIMER_CTRL_DIV64        (6 << 4)
#define TIMER_CTRL_DIV128       (7 << 4)
#define TIMER_CTRL_PERIODIC     (0 << 7)
#define TIMER_CTRL_ONESHOT      (1 << 7)
#define TIMER_CTRL_CLK_OSC24M	(1 << 2)

#define TIMER_IRQ_EN_T0         (1 << 0)
#define TIMER_IRQ_EN_T1         (1 << 1)

#define TIMER_IRQ_ST_T0         (1 << 0)
#define TIMER_IRQ_ST_T1         (1 << 1)

#define TIMER_IRQ_CLR		(3)

struct spectimer {
	volatile uint32_t timercontrol;
	volatile uint32_t timerintvalue;
	volatile uint32_t timercurvalue;
	volatile uint32_t _padding;
};

typedef struct {
	volatile uint32_t timer_irqen;	/* 0x00 */
	volatile uint32_t timer_status;
	volatile uint32_t _padding[2];
	struct spectimer timers[2];
} sun4i_timer_t;

#endif /* SUN4I_TIMER_H */
