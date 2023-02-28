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

#ifndef _VBWA88PG_H_
#define _VBWA88PG_H_

#include <soo/dev/vknx.h>

#define DATAPOINT_COUNT             0x06

#define VBWA88PG_BLIND_UP           0x00
#define VBWA88PG_BLIND_DOWN         0x01
#define VBWA88PG_BLIND_INC          0x00
#define VBWA88PG_BLIND_DEC          0x01

/**
 * @brief BAOS datapoint command codes. See datasheet
 * 
 */
typedef enum {
    NO_COMMAND =                            0b0000,
    SET_NEW_VALUE =                         0b0001,
    SEND_VALUE_ON_BUS =                     0b0010,
    SET_NEW_VALUE_AND_SEND_ON_BUS =         0b0011,
    READ_NEW_VALUE_VIA_BUS =                0b0100,
    CLEAR_DATA_POINT_TRANSMISSION_STATE =   0b0101
} datapoint_commands;

/** 
 * @brief Datapoint functionalities 
 */
typedef enum {
    UP_DOWN = 0,
    INC_DEC_STOP,
    BLIND_SET_POS,
    SLAT_SET_POS,
    BLIND_GET_POS,
    SLAT_GET_POS
} dp_function;

/** 
 * @brief Struct representing VBWA88PG blind
 * 
 * @param dps BAOS datapoint array.
 * @param blind_id Address of the first BAOS datapoint. It is implied that the others have been
 *      setted to be incremental from the first one. The configuration is done using ETS.
 * @param time private value. Store the time at witch a button press is detected
 * @param prev_cmd store the last received command.
 */
typedef struct {
    dp_t dps[DATAPOINT_COUNT];
    uint16_t blind_id;
    uint64_t time;
    uint8_t prev_cmd;
} blind_vbwa88pg_t;

/**
 * @brief Initialize the blind_vbwa88pg_t struct
 * 
 * @param blind Blind to initialize.
 * @param first_dp_id Address of the first datapoint of the blind.
 */
void vbwa88pg_blind_init(blind_vbwa88pg_t *blind, uint16_t first_dp_id);

/**
 * @brief Update datapoint of the blind when receiving data from KNX.
 * 
 * @param blind Blind to update
 * @param dps Datapoints array coming from KNX
 * @param dp_count Number of datapoints in the array
 */
void vbwa88pg_blind_update(blind_vbwa88pg_t *blind, dp_t *dps, int dp_count);

/**
 * @brief  Move the blind up or down
 * @param  blind: blind to apply the movement to
 * @retval None
 */
void vbwa88pg_blind_up_down(blind_vbwa88pg_t *blind);

/**
 * @brief  Move the blind by a step or stop it if it is already moving
 * @param  blind: blind to apply the movement to
 * @retval None
 */
void vbwa88pg_blind_inc_dec_stop(blind_vbwa88pg_t *blind);

#endif // _VBWA88PG_H_