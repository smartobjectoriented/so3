/*
 * Copyright (C) 2016 Daniel Rossier <daniel.rossier@soo.tech>
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

#ifndef __VCPU_H__
#define __VCPU_H__

#include <avz/uapi/avz.h>

/*
 * Initialise a VCPU. Each VCPU can be initialised only once. A 
 * newly-initialised VCPU will not run until it is brought up by VCPUOP_up.
 * 
 * @extra_arg == pointer to vcpu_guest_context structure containing initial
 *               state for the VCPU.
 */
#define VCPUOP_initialise            0

/*
 * Bring up a VCPU. This makes the VCPU runnable. This operation will fail
 * if the VCPU has not been initialised (VCPUOP_initialise).
 */
#define VCPUOP_up                    1

/*
 * Bring down a VCPU (i.e., make it non-runnable).
 * There are a few caveats that callers should observe:
 *  1. This operation may return, and VCPU_is_up may return false, before the
 *     VCPU stops running (i.e., the command is asynchronous). It is a good
 *     idea to ensure that the VCPU has entered a non-critical loop before
 *     bringing it down. Alternatively, this operation is guaranteed
 *     synchronous if invoked by the VCPU itself.
 *  2. After a VCPU is initialised, there is currently no way to drop all its
 *     references to domain memory. Even a VCPU that is down still holds
 *     memory references via its pagetable base pointer and GDT. It is good
 *     practise to move a VCPU onto an 'idle' or default page table, LDT and
 *     GDT before bringing it down.
 */
#define VCPUOP_down                  2

/* Returns 1 if the given VCPU is up. */
#define VCPUOP_is_up                 3

/* VCPU is currently running on a physical CPU. */
#define RUNSTATE_running  0

/* VCPU is runnable, but not currently scheduled on any physical CPU. */
#define RUNSTATE_runnable 1

/* VCPU is blocked (a.k.a. idle). It is therefore not runnable. */
#define RUNSTATE_blocked  2

/* VCPU is offline, not blocked and not runnable */
#define RUNSTATE_offline  3

/*
 * Set or stop a VCPU's periodic timer. Every VCPU has one periodic timer
 * which can be set via these commands. Periods smaller than one millisecond
 * may not be supported.
 */
#define VCPUOP_set_periodic_timer    6 /* arg == vcpu_set_periodic_timer_t */
#define VCPUOP_stop_periodic_timer   7 /* arg == NULL */
struct vcpu_set_periodic_timer {
    uint64_t period_ns;
};
typedef struct vcpu_set_periodic_timer vcpu_set_periodic_timer_t;

/*
 * Set or stop a VCPU's single-shot timer. Every VCPU has one single-shot
 * timer which can be set via these commands.
 */
#define VCPUOP_set_singleshot_timer  8 /* arg == vcpu_set_singleshot_timer_t */
#define VCPUOP_stop_singleshot_timer 9 /* arg == NULL */
struct vcpu_set_singleshot_timer {
    uint64_t timeout_abs_ns;   /* Absolute system time value in nanoseconds. */
    uint32_t flags;            /* VCPU_SSHOTTMR_??? */
};
typedef struct vcpu_set_singleshot_timer vcpu_set_singleshot_timer_t;


#endif /* __VCPU_H__ */

