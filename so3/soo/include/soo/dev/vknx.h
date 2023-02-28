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

#ifndef VKNX_H
#define VKNX_H

#include <soo/ring.h>
#include <soo/grant_table.h>
#include <soo/vdevfront.h>


#define VKNX_NAME		                    "vknx"
#define VKNX_PREFIX		                    "[" VKNX_NAME " frontend] "

/** Max data bytes in a datapoint **/
#define VKNX_DATAPOINT_DATA_MAX_SIZE        14

/** 
 * Max datapoints. The Kberry supports a maximum of 1000 but only 10 are supported 
 * because of the 1 page max size of the ring memory 
 */
#define VKNX_MAX_DATAPOINTS                 10

/**
 * @brief  Type of messages coming from the Kberry 
 * @param KNX_RESPONSE Response to a request.
 * @param KNX_INDICATION Event on the bus. For example a switch press.   
 */
typedef enum {
    KNX_RESPONSE = 0,
    KNX_INDICATION
} event_type;

/**
 * @brief  Request type. Only 2 requests are implemented. Many others are available
 * @param GET_DP_VALUE Get the value of a datapoint   
 * @param SET_DP_VALUE Set the value of a datapoint
 */
typedef enum {
    GET_DP_VALUE = 0,
    SET_DP_VALUE
} request_type;

/**
 * @brief  Structure representing a datapoint.
 * @param id Id of the datapoint setted using ETS6.  
 * @param state the state of the datapoint. Only in responses.
 * @param cmd The command to apply to the datapoint. Only in requests.
 * @param data_len Length of the data array.
 * @param data Array containing the data of the datapoint.
 */
struct dp {
    uint16_t id;
    union {
        uint8_t state;
        uint8_t cmd;
    };
    uint8_t data_len;
    uint8_t data[VKNX_DATAPOINT_DATA_MAX_SIZE];
};
typedef struct dp dp_t;

/**
 * @brief  Ring request.
 * @param type Type of the request.
 * @param dp_count Number of datapoint in the request.   
 * @param datapoints An array of datapoints concerned by the request.
 */
typedef struct {
    request_type type;
    uint16_t dp_count;
    dp_t datapoints[VKNX_MAX_DATAPOINTS];
} vknx_request_t;

/**
 * @brief  Ring response.
 * @param type Type of the response.
 * @param dp_count Number of datapoint in the response.  
 * @param datapoints An array of datapoints concerned by the response.
 */
typedef struct  {
    event_type event;
    uint16_t dp_count;
    dp_t datapoints[VKNX_MAX_DATAPOINTS];
} vknx_response_t;



/*
 * Generate ring structures and types.
 */
DEFINE_RING_TYPES(vknx, vknx_request_t, vknx_response_t);

/*
 * General structure for this virtual device (frontend side)
 */

typedef struct {
	/* Must be the first field */
	vdevfront_t vdevfront;

	vknx_front_ring_t ring;
	unsigned int irq;

	grant_ref_t ring_ref;
	grant_handle_t handle;
	uint32_t evtchn;

} vknx_t;

/**
 * @brief Bloking function that waits on data coming from the KNX backend
 * @param data: Pointer to the data received from the backend.
 * @retval 0 if successful, -1 if no data could be read.
 */
int get_knx_data(vknx_response_t *data);

/**
 * @brief Set the value of a datapoint or multiples datapoints 
 * @param  dps: Array of datapoints
 * @param  dp_count: Number of datapoints
 * @retval None
 */
void vknx_set_dp_value(dp_t* dps, int dp_count);

/**
 * @brief Get datapoints value.
 * @param  first_dp: First datapoint id. 
 * @param  dp_count: Number of datapoints to get the value of starting from first_dp.
 * @retval None
 */
void vknx_get_dp_value(uint16_t first_dp, int dp_count);

#endif /* BLIND_H */
