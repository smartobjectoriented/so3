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

#if 0
#define DEBUG
#endif

#include <soo/knx/vbwa88pg.h>
#include <timer.h>
#include <soo/debug.h>

void vbwa88pg_blind_init(blind_vbwa88pg_t *blind, uint16_t first_dp_id) {
    int i;

    blind->blind_id = first_dp_id;

    for (i = 0; i < DATAPOINT_COUNT; i++) {
        blind->dps[i].id = first_dp_id + i;
        blind->dps[i].cmd = SET_NEW_VALUE_AND_SEND_ON_BUS;
        blind->dps[i].data_len = 1;
        blind->dps[i].data[0] = 0x00;
    }
}

void vbwa88pg_blind_update(blind_vbwa88pg_t *blind, dp_t *dps, int dp_count) {
    int i, j;

    for (i = 0; i < dp_count; i++) {
        for (j = 0; j < DATAPOINT_COUNT; j++) {
            if (dps[i].id == blind->dps[j].id) {
                memcpy(blind->dps[j].data, dps[i].data, VKNX_DATAPOINT_DATA_MAX_SIZE);
            } 
        }
    }
#ifdef DEBUG
    vknx_print_dps(dps, dp_count);
#endif
}

/**
 * @brief Effectively send data to KNX frontend. Move up or down
 * 
 * @param blind Blind to control
 */
void vbwa88pg_blind_up_down(blind_vbwa88pg_t *blind) {
    dp_t dps[1];
    dps[0] = blind->dps[UP_DOWN];
    vknx_set_dp_value(dps, 1);
}

/**
 * @brief Effectively send data to KNX frontend. Increase or decrease by one step or stop the blind
 * 
 * @param blind Blind to control
 */
void vbwa88pg_blind_inc_dec_stop(blind_vbwa88pg_t *blind) {
    dp_t dps[1];
    dps[0] = blind->dps[INC_DEC_STOP];
    vknx_set_dp_value(dps, 1);
}
