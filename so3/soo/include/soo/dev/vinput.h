/*
 * Copyright (C) 2016-2018 Baptiste Delporte <bonel@bonel.net>
 * Copyright (C) 2018-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef VINPUT_H
#define VINPUT_H

#include <soo/ring.h>
#include <soo/vdevfront.h>
#include <soo/gnttab.h>

#define VINPUT_NAME "vinput"
#define VINPUT_PREFIX "[" VINPUT_NAME "] "

#define SRC_UNKNOWN -1
#define SRC_MOUSE 0
#define SRC_KEYBOARD 1

typedef struct {
	/* No request */
} vinput_request_t;

typedef struct {
	unsigned int type;
	unsigned int code;
	int value;
} vinput_response_t;

DEFINE_RING_TYPES(vinput, vinput_request_t, vinput_response_t);

typedef struct {
	/* Must be the first field */
	vdevfront_t vdevfront;

	vinput_front_ring_t ring;
	unsigned int irq;

	grant_ref_t ring_ref;
	uint32_t evtchn;

} vinput_t;

void soo_mse_event(unsigned int type, unsigned int code, int value);
void soo_input_event(unsigned int type, unsigned int code, int value);

#endif /* VINPUT_H */
