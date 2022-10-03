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

#ifndef COMPLETION_H
#define COMPLETION_H

#include <spinlock.h>
#include <list.h>
#include <schedule.h>

/*
 * Simple completion structure to perform synchronized operations between threads.
 */
struct completion {
	volatile uint32_t count;
	struct list_head tcb_list;
};
typedef struct completion completion_t;

/**
 * reinit_completion - reinitialize a completion structure
 * @x:  pointer to completion structure that is to be reinitialized
 *
 * This inline function should be used to reinitialize a completion structure so it can
 * be reused. This is especially important after complete_all() is used.
 */
static inline void reinit_completion(struct completion *x)
{
	x->count = 0;
}

void wait_for_completion(completion_t *completion);
void complete(completion_t *completion);
void init_completion(completion_t *completion);

#endif /* COMPLETION_H */
