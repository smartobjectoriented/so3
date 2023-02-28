/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2022 Mattia Gallacchi <mattia.gallaccchi@heig-vd.ch>
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

#ifndef VENOCEAN_H
#define VENOCEAN_H

#include <soo/ring.h>
#include <soo/grant_table.h>
#include <soo/vdevfront.h>

/*** For the moment it should be enough ***/
#define VENOCEAN_BUFFER_SIZE	256

#define VENOCEAN_NAME			"venocean"
#define VENOCEAN_PREFIX			"[" VENOCEAN_NAME " frontend ]"

//different states for the switchs (single channel)
//Interpreted from the frame during successive tests (could not find documentation about the data encription)

typedef struct {
	unsigned char buffer[VENOCEAN_BUFFER_SIZE];
	int len;
} venocean_request_t;

typedef struct  {
	unsigned char buffer[VENOCEAN_BUFFER_SIZE];
	int len;
} venocean_response_t;

/*
 * Generate ring structures and types.
 */
DEFINE_RING_TYPES(venocean, venocean_request_t, venocean_response_t);

/*
 * General structure for this virtual device (backend side)
 */
typedef struct {
	vdevfront_t vdevfront;

	venocean_front_ring_t ring;
	unsigned int irq;

	grant_ref_t ring_ref;
	grant_handle_t handle;
	uint32_t evtchn;

} venocean_t;

/**
 * @brief Get data from En0cean backend. Call is blocking until data is ready.
 * 
 * @param buf Buffer to store data to. Must preallocated with at least 256 bytes.
 * @return int data length, -1 if error 
 */
int venocean_get_data(char *buf);

#endif /* VENOCEAN_H */
