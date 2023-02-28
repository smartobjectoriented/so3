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

#ifndef _GTL2TW_H_
#define _GTL2TW_H_ 

#include <types.h>

#define DP_COUNT        4

/** 
 *  First datapoint address as defined by ETS. It is assumed than the others are defined 
 *  starting from the first one. Ex. 0x7, 0x8, 0x9, etc...
 */
#define FIRST_DP_ID     0x07
#define SECOND_DP_ID    (FIRST_DP_ID + 1)
#define THIRD_DP_ID     (FIRST_DP_ID + 2)  
#define FORTH_DP_ID     (FIRST_DP_ID + 3)  

typedef unsigned char byte;

typedef struct {
    bool status[DP_COUNT];
    bool events [DP_COUNT];
} gtl2tw_t;

/**
 * @brief  Initialize switch GTL2TW struct
 * @param  sw: Switch struct to init. 
 * @retval None
 */
void gtl2tw_init(gtl2tw_t *sw);

/**
 * @brief  Wait for a switch event. This function is blocking
 * @param  sw: Switch struct in which to store the event
 * @retval None
 */
void gtl2tw_wait_event(gtl2tw_t *sw);

#endif // GTL2TW