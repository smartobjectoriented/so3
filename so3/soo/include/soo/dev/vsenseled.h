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

#ifndef VSENSELED_H
#define VSENSELED_H

#include <soo/ring.h>
#include <soo/grant_table.h>
#include <soo/vdevfront.h>

#define VSENSELED_PACKET_SIZE	32

#define VSENSELED_NAME		"vsenseled"
#define VSENSELED_PREFIX	"[" VSENSELED_NAME "] "

typedef struct {
	int lednr;
	bool ledstate; /* true = on, false = off */
} vsenseled_request_t;

typedef struct  {
	/* nothing */
} vsenseled_response_t;

/*
 * Generate ring structures and types.
 */
DEFINE_RING_TYPES(vsenseled, vsenseled_request_t, vsenseled_response_t);

/*
 * General structure for this virtual device (frontend side)
 */

typedef struct {
	/* Must be the first field */
	vdevfront_t vdevfront;

	vsenseled_front_ring_t ring;
	unsigned int irq;

	grant_ref_t ring_ref;
	grant_handle_t handle;
	uint32_t evtchn;

} vsenseled_t;

/* API for the user of this frontend */

void vsenseled_set(int lednr, bool ledstate);

#endif /* VSENSELED_H */
