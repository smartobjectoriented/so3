/*
 * Copyright (C) 2025 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
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

#ifndef VLOGS_H
#define VLOGS_H

#include <soo/ring.h>
#include <soo/vdevfront.h>
#include <soo/gnttab.h>

#define VLOGS_NAME "vlogs"
#define VLOGS_PREFIX "[" VLOGS_NAME "] "

typedef struct {
	char c;
} vlogs_request_t;

typedef struct {
	char c;
} vlogs_response_t;

DEFINE_RING_TYPES(vlogs, vlogs_request_t, vlogs_response_t);

typedef struct {
	/* Must be the first field */
	vdevfront_t vdevfront;

	vlogs_front_ring_t ring;
	unsigned int irq;

	grant_ref_t ring_ref;
	uint32_t evtchn;

} vlogs_t;


/* vlogs cdev initialisation */
void vlogs_cdev_init(dev_t *dev);

bool vlogs_ready(void);

void vlogs_write(char *buffer, int count);

#endif /* VLOGS_H */

