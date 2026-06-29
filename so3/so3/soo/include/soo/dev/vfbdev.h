/*
 * Copyright (C) 2026 Clément Dieperink <clement.dieperink@heig-vd.ch>
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

#ifndef VFBDEV_H
#define VFBDEV_H

#include <soo/ring.h>
#include <soo/vdevfront.h>
#include <soo/gnttab.h>

#define VFBDEV_NAME "vfbdev"
#define VFBDEV_PREFIX "[" VFBDEV_NAME "] "

typedef struct {
	/* Nothing */
} vfbdev_request_t;

typedef struct {
	uint32_t hres;
	uint32_t vres;
	uint32_t bpp;
	uint64_t memory_size;
} vfbdev_response_t;

DEFINE_RING_TYPES(vfbdev, vfbdev_request_t, vfbdev_response_t);

typedef struct {
	/* Must be the first field */
	vdevfront_t vdevfront;

	vfbdev_front_ring_t ring;
	unsigned int irq;

	grant_ref_t ring_ref;
	uint32_t evtchn;
} vfbdev_t;

#endif /* VFBDEV_H */
