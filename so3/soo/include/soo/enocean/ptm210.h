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

#ifndef _PTM210_H_
#define _PTM210_H_ 

#include <types.h>
#include <timer.h>
#include <completion.h>
#include <thread.h>
#include <asm/atomic.h>

#define PTM210_SWITCH_UP             0x70
#define PTM210_SWITCH_DOWN           0x50
#define PTM210_SWITCH_RELEASED       0x00
#define PTM210_PRESSED_TIME_MS       300

typedef unsigned char byte;

/**
 * @brief PTM210 enocean switch struct
 * 
 * @param id Enocean unique ID
 * @param up Switch up pressed
 * @param down Switch down pressed
 * @param released Switch released
 * @param event Switch event. One of the above is set to true
 * @param _pressed_time Timer used to discriminate a long press from a short
 * @param _wait_event Completion 
 */
typedef struct {
    uint32_t id;
    atomic_t up;
    atomic_t down;
    atomic_t released;
    atomic_t event;

    // private
    timer_t _pressed_time;
    struct completion _wait_event;
    tcb_t *_wait_event_th;
    atomic_t _th_run;

} ptm210_t;


/**
 * @brief Initialize PTM210 struct members
 * 
 * @param sw PTM210 switch to initialize
 * @param switch_id Enocean id. Read on the back of the device
 */
void ptm210_init(ptm210_t *sw, uint32_t switch_id);

/**
 * @brief Stop the event thread
 * @param  sw: Switch to stop listening for 
 * @retval None
 */
void ptm210_deinit(ptm210_t *sw);

/**
 * @brief Reset switch values. Not the id. After an event is received and treated this function 
 * must be called. 
 * 
 * @param sw switch to reset the values of
 */
void ptm210_reset(ptm210_t *sw);

#endif //_PTM210_H_