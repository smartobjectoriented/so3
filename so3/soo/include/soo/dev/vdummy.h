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

#ifndef VDUMMY_H
#define VDUMMY_H

#include <soo/ring.h>
#include <soo/gnttab.h>
#include <soo/vdevfront.h>

#define VDUMMY_PACKET_SIZE	32

#define VDUMMY_NAME		"vdummy"
#define VDUMMY_PREFIX		"[" VDUMMY_NAME "] "

typedef struct {
	char buffer[VDUMMY_PACKET_SIZE];
} vdummy_request_t;

typedef struct  {
	char buffer[VDUMMY_PACKET_SIZE];
} vdummy_response_t;

/*
 * Generate ring structures and types.
 */
DEFINE_RING_TYPES(vdummy, vdummy_request_t, vdummy_response_t);

/*
 * General structure for this virtual device (frontend side)
 */

typedef struct {
	/* Must be the first field */
	vdevfront_t vdevfront;

	vdummy_front_ring_t ring;
	unsigned int irq;

	grant_ref_t ring_ref;
	
	uint32_t evtchn;

} vdummy_t;


#endif /* VDUMMY_H */
