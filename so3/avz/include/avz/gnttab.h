/*
 * Copyright (C) 2024 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef GNTTAB_H
#define GNTTAB_H

#include <list.h>

#include <soo/uapi/soo.h>

struct gnttab {
	struct list_head list; /* List of grant pages */

	domid_t origin_domid; /* Domain which provides the grant */
	domid_t target_domid; /* Target domain (granted to) */

	/* (Real) physical frame number to be granted */
	addr_t pfn;

	/* Unique ref ID used by the domain which refers to this page */
	grant_ref_t ref;
};
typedef struct gnttab gnttab_t;

void gnttab_init(struct domain *d);
void do_gnttab(gnttab_op_t *args);
addr_t map_vbstore_pfn(int target_domid, int pfn);

#endif /* GNTTAB_H */
