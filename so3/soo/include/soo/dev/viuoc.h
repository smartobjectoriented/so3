/*
 * Copyright (C) 2023 A.Gabriel Catel Torres <arzur.cateltorres@heig-vd.ch>
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
 * Description: This file is the header of the front end for the IUOC 
 * application. There is the definition of the structure used to transfer
 * and decode the data. There are also functions to send and retrieve any
 * data that will be used to control smart objects.
 */

#ifndef VIUOC_H
#define VIUOC_H

#include <soo/ring.h>
#include <soo/grant_table.h>
#include <soo/vdevfront.h>

#define DEFAULT_DIM_VALUE		0
#define NB_DATA_MAX 			15

typedef enum {
	SWITCH = 0,
	BLIND,
	CMD_NONE
} iuoc_cmd_t;

#define VIUOC_NAME		    "viuoc"
#define VIUOC_PREFIX		"[" VIUOC_NAME " frontend] "

/* Global communication declaration for IUOC */
/* This needs to match the declaration done in the backend*/
typedef enum {
	IUOC_ME_BLIND,
	IUOC_ME_OUTDOOR,
	IUOC_ME_WAGOLED,
	IUOC_ME_HEAT,
	IUOC_ME_SWITCH,
	IUOC_ME_END
} me_type_t;

/* Structure used to define a single data defining any behaviour/information
 * coming from a SO. Multiple data can be sent from the SO.
 */
typedef struct {
	char name[30];
	char type[30];
	int value;
} field_data_t;

/* Generic structure used to pass any ME information between the FE and the user space */
typedef struct {
	me_type_t me_type;
	unsigned timestamp;
    unsigned data_array_size;
	field_data_t data_array[NB_DATA_MAX];
} iuoc_data_t;

typedef struct {
	iuoc_data_t me_data;
} viuoc_request_t;

typedef struct  {
	iuoc_data_t me_data;
} viuoc_response_t;

/*
 * Generate ring structures and types.
 */
DEFINE_RING_TYPES(viuoc, viuoc_request_t, viuoc_response_t);

/*
 * General structure for this virtual device (frontend side)
 */
typedef struct {
	/* Must be the first field */
	vdevfront_t vdevfront;

	viuoc_front_ring_t ring;
	unsigned int irq;

	grant_ref_t ring_ref;
	grant_handle_t handle;
	uint32_t evtchn;

} viuoc_t;


/**
 * @brief Send a command to viuoc backend
 * 
 * @param cmd Command to execute. See wago_cmd_t enum
 * @return int 0 if success, -1 if error
 */
int viuoc_set(iuoc_data_t me_data);

/**
 * @brief Bloking function that waits on data coming from the IUOC backend
 * 
 * @param data: Pointer to the data received from the backend.
 * @retval 0 if successful, -1 if no data could be read.
 */
int get_iuoc_me_data(iuoc_data_t *data);


#endif /* WAGOLED_H */
