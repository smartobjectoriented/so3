/*
 * Copyright (C) 2021 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef LEDCTRL_H
#define LEDCTRL_H

#include <completion.h>
#include <spinlock.h>

#include <me/common.h>

/*
 * Never use lock (completion, spinlock, etc.) in the shared page since
 * the use of ldrex/strex instructions will fail with cache disabled.
 */
typedef struct {

	int local_nr;
	int incoming_nr;

	/* To determine if the ME needs to be propagated.
	 * If it is the same state, no need to be propagated.
	 */
	bool need_propagate;

	uint64_t initiator;

	/*
	 * MUST BE the last field, since it contains a field at the end which is used
	 * as "payload" for a concatened list of hosts.
	 */
	me_common_t me_common;

} sh_ledctrl_t;

/* Export the reference to the shared content structure */
extern sh_ledctrl_t *sh_ledctrl;

extern struct completion upd_lock;

void propagate(void);

#endif /* LEDCTRL_H */


