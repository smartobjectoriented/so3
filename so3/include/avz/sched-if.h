/*
 * Copyright (C) 2016 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef __SCHED_IF_H__
#define __SCHED_IF_H__

#include <percpu.h>

struct schedule_data {
    spinlock_t    schedule_lock;  /* spinlock protecting curr        */
    struct timer  s_timer;        /* scheduling timer                */

    unsigned int current_dom;

};

struct task_slice {
	struct domain *d;
	u64 time;
};

struct scheduler {
	char *name;             /* full name for this scheduler      */

	void (*init)  (void);
	void (*sleep) (struct domain *);
	void (*wake)  (struct domain *);

	struct task_slice (*do_schedule) (void);

	struct schedule_data sched_data;
};

extern struct scheduler sched_flip;
extern struct scheduler sched_agency;

#endif /* __SCHED_IF_H__ */
