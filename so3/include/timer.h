/*
 * Copyright (C) 2014-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef TIMER_H
#define TIMER_H

#include <types.h>
#include <string.h>

#include <device/timer.h>

#define NSECS		1000000000ull

#define NOW()           ((u64) get_s_time())
#define SECONDS(_s)     ((u64)((_s)  * 1000000000ull))
#define MILLISECS(_ms)  ((u64)((_ms) * 1000000ull))
#define MICROSECS(_us)  ((u64)((_us) * 1000ull))
#define STIME_MAX       ((u64)(~0ull))

struct timer {
    /* System time expiry value (nanoseconds since boot). */
    u64 expires;

    /* Linked list. */
    struct timer *list_next;

    /* On expiry, '(*function)(data)' will be executed in softirq context. */
    void (*function)(void *);

    /* Some timer might be initialized on CPU #1 or #2, and in this case, they rely on the CPU #0 timer */
    struct timer *sibling;
    int timer_cpu;

    void *data;

    /* CPU on which this timer will be installed and executed. */
    uint16_t cpu;

    /* Timer status. */
#define TIMER_STATUS_inactive  0  /* Not in use; can be activated.    */
#define TIMER_STATUS_killed    1  /* Not in use; canot be activated.  */
#define TIMER_STATUS_in_list   2  /* In use; on overflow linked list. */

    uint8_t status;
};
typedef struct timer timer_t;

/*
 * All functions below can be called for any CPU from any CPU in any context.
 */

/*
 * Returns TRUE if the given timer is on a timer list.
 * The timer must *previously* have been initialised by init_timer(), or its
 * structure initialised to all-zeroes.
 */
static inline int active_timer(struct timer *timer)
{
    return (timer->status == TIMER_STATUS_in_list);
}

/*
 * Initialise a timer structure with an initial callback CPU, callback
 * function and callback data pointer. This function may be called at any
 * time (and multiple times) on an inactive timer. It must *never* execute
 * concurrently with any other operation on the same timer.
 */
static inline void init_timer(struct timer *timer, void (*function)(void *), void *data, unsigned int cpu)
{
    memset(timer, 0, sizeof(*timer));

    timer->function = function;
    timer->data     = data;
    timer->cpu      = cpu;
}

/*
 * Set the expiry time and activate a timer. The timer must *previously* have
 * been initialised by init_timer() (so that callback details are known).
 */
extern void set_timer(struct timer *timer, u64 expires);

/*
 * Deactivate a timer This function has no effect if the timer is not currently
 * active.
 * The timer must *previously* have been initialised by init_timer(), or its
 * structure initialised to all zeroes.
 */
extern void stop_timer(struct timer *timer);

/*
 * Deactivate a timer and prevent it from being re-set (future calls to
 * set_timer will silently fail). When this function returns it is guaranteed
 * that the timer callback handler is not running on any CPU.
 * The timer must *previously* have been initialised by init_timer(), or its
 * structure initialised to all zeroes.
 */
extern void kill_timer(struct timer *timer);

/*
 * Bootstrap initialisation. Must be called before any other timer function.
 */
extern void timer_init(void);


/* Arch-defined function to reprogram timer hardware for new deadline. */
extern void reprogram_timer(u64 deadline);

void clocks_calc_mult_shift(u32 *mult, u32 *shift, u32 from, u32 to, u32 maxsec);

int do_nanosleep(const struct timespec *req, struct timespec *rem);

int do_get_time_of_day(struct timespec *tv);
int do_get_clock_time(int clk_id, struct timespec *ts);

#ifdef CONFIG_AVZ

struct domain;

extern void send_timer_event(struct domain *d);

void domain_set_time_offset(struct domain *d, int32_t time_offset_seconds);

#endif /* CONFIG_AVZ */

#endif /* TIMER_H */
