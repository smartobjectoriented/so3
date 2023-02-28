/*
 * Copyright (C) 2014-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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


#ifndef DEVICE_TIMER_H
#define DEVICE_TIMER_H

#include <types.h>

#include <device/device.h>

#define CLOCKSOURCE_MASK(bits) (u64)(bits<64 ? ((1ull<<bits)-1) : -1)

/* Time conversion units */

struct timespec {
	time_t		tv_sec;			/* seconds */
	time_t		tv_nsec;		/* nanoseconds */
};

struct timeval {
	time_t		tv_sec;			/* seconds */
	time_t		tv_usec;		/* microseconds */
};

/* All timing information below must be express in nanoseconds. The underlying hardware is responsible
 * to perform the necessary alignment on 64 bits. */

/* Structure for a periodic timer */
typedef struct {
	dev_t *dev;	/* Pointer to the periodic timer driver */

	uint64_t period; /* Period in ns of the periodic timer */

	void (*start)(void);
	void (*stop)(void);

} periodic_timer_t;

/* Structure for a oneshot timer */
typedef struct {
	dev_t *dev;	/* Pointer to the oneshot timer driver */

	void (*set_delay)(uint64_t delay_ns);
	void (*start)(void);

	u32 mult;
	u32 shift;

	u64 max_delta_ns;
	u64 min_delta_ns;

} oneshot_timer_t;

/* Structure of a clocksource timer */
typedef struct {
	dev_t *dev;	/* Pointer to the clocksource timer driver */

	u64 (*read)(void);

	uint32_t rate;

	u64 mask;
	u32 mult;
	u32 shift;

	/*
	 * Second part is written at each timer interrupt
	 * Keep it in a different cache line to dirty no
	 * more than one cache line.
	 */
	u64 cycle_last;

} clocksource_timer_t;

extern periodic_timer_t periodic_timer;
extern oneshot_timer_t oneshot_timer;
extern clocksource_timer_t clocksource_timer;

/**
 * cyc2ns - converts clocksource cycles to nanoseconds
 * @cs:		Pointer to clocksource
 * @cycles:	Cycles
 *
 * Uses the clocksource and ntp ajdustment to convert cycle_ts to nanoseconds.
 *
 * XXX - This could use some mult_lxl_ll() asm optimization
 */
static inline u64 cyc2ns(u64 cycles)
{
	return ((u64) cycles * clocksource_timer.mult) >> clocksource_timer.shift;
}

u64 get_s_time(void);

void timer_dev_init(void);
bool timer_dev_set_deadline(u64 deadline);

void secondary_timer_init(void);

#ifdef CONFIG_AVZ
void timer_interrupt(bool periodic);
#endif /* CONFIG_AVZ */

#endif /* DEVICE_TIMER_H */
