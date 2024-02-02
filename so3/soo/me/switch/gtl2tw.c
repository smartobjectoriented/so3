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

#include <string.h>

#include <soo/knx/gtl2tw.h>
#include <soo/dev/vknx.h>

#define DATAPOINT_1     0x00
#define DATAPOINT_2     0x01
#define DATAPOINT_3     0x02
#define DATAPOINT_4     0x03

/**
 * @brief  Enum matching BAOS addresses to physical positions
 */
enum {
    GTL2TW_UP_LEFT = FIRST_DP_ID,
    GTL2TW_DOWN_LEFT = SECOND_DP_ID,
    GTL2TW_UP_RIGHT = THIRD_DP_ID,
    GTL2TW_DOWN_RIGHT = FORTH_DP_ID
} ids;

/**
 * @brief  Enum describing the status ON/OFF sent by the switch.
 */
enum {
    OFF = 0x00,
    ON
} sts;

void gtl2tw_init(gtl2tw_t *sw) {
    memset(sw->status, 0, DP_COUNT);
    memset(sw->events, 0, DP_COUNT);
}

void gtl2tw_wait_event(gtl2tw_t *sw) {
    vknx_response_t data;
    int i;

    memset(sw->events, 0, DP_COUNT);

    if (get_knx_data(&data) < 0) {
		DBG("Failed to get knx data\n");
		return;
	} 

	DBG("Got new knx data. Type:\n");

	switch (data.event)
	{
		case KNX_RESPONSE:
			DBG("KNX response\n");
			break;
		
		case KNX_INDICATION:
			DBG("KNX indication\n");
			
            for (i = 0; i < data.dp_count; i++) {
                switch(data.datapoints[i].id) {
                    case GTL2TW_UP_LEFT:
                        DBG("UP left\n");
                        if (sw->status[DATAPOINT_1] != data.datapoints[i].data[0]) {
                            sw->events[DATAPOINT_1] = true;
                            sw->status[0] = sw->status[DATAPOINT_1] == ON ? OFF : ON;
                        }
                        break;
                    case GTL2TW_DOWN_LEFT:
                        DBG("DOWN left\n");
                        if (sw->status[DATAPOINT_2] != data.datapoints[i].data[0]) {
                            sw->events[DATAPOINT_2] = true;
                            sw->status[DATAPOINT_2] = sw->status[DATAPOINT_2] == ON ? OFF : ON;
                        }
                        break;

                    case GTL2TW_UP_RIGHT:
                        DBG("UP right\n");
                        if (sw->status[DATAPOINT_3] != data.datapoints[i].data[0]) {
                            sw->events[DATAPOINT_3] = true;
                            sw->status[DATAPOINT_3] = sw->status[DATAPOINT_3] == ON ? OFF : ON;
                        }
                        break;
                    case GTL2TW_DOWN_RIGHT:
                        DBG("DOWN right\n");
                        if (sw->status[DATAPOINT_4] != data.datapoints[i].data[0]) {
                            sw->events[DATAPOINT_4] = true;
                            sw->status[DATAPOINT_4] = sw->status[DATAPOINT_4] == ON ? OFF : ON;
                        }
                        break;
                }
            }

			break;
	}
}