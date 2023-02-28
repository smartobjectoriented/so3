/*
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

#ifndef VWAGOLED_H
#define VWAGOLED_H

#include <soo/ring.h>
#include <soo/grant_table.h>
#include <soo/vdevfront.h>

#define DEFAULT_DIM_VALUE		0

typedef enum {
	/* Turn led on */
	LED_ON = 0,
	/* Turn led off*/
	LED_OFF,

	/*** !!! Not yet implemented, ***/
	/* Get current status */
	GET_STATUS,
	/* Get devices connected to the DALI bus */
	GET_TOPOLOGY,
	
	CMD_NONE
} wago_cmd_t;

/*** Normally DALI master can manage up to 64 devices ***/
#define VWAGOLED_PACKET_SIZE	64

#define VWAGOLED_NAME		"vwagoled"
#define VWAGOLED_PREFIX		"[" VWAGOLED_NAME " frontend] "

typedef struct {
	uint8_t cmd;
	uint8_t dim_value;
	int ids[VWAGOLED_PACKET_SIZE];
	uint8_t ids_count;
} vwagoled_request_t;

typedef struct  {
	uint8_t cmd;
	uint8_t dim_value;
	int ids[VWAGOLED_PACKET_SIZE];
	uint8_t ids_count;
} vwagoled_response_t;

/*
 * Generate ring structures and types.
 */
DEFINE_RING_TYPES(vwagoled, vwagoled_request_t, vwagoled_response_t);

/*
 * General structure for this virtual device (frontend side)
 */

typedef struct {
	/* Must be the first field */
	vdevfront_t vdevfront;

	vwagoled_front_ring_t ring;
	unsigned int irq;

	grant_ref_t ring_ref;
	grant_handle_t handle;
	uint32_t evtchn;

} vwagoled_t;


/**
 * @brief Send a command to vwagoled backend
 * 
 * @param ids Leds ids to apply the command to.
 * @param size Number of ids in the ids array
 * @param cmd Command to execute. See wago_cmd_t enum
 * @return int 0 if success, -1 if error
 */
int vwagoled_set(int *ids, int size, wago_cmd_t cmd);

#endif /* WAGOLED_H */
