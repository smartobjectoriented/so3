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

#ifndef _ENOCEAN_H_
#define _ENOCEAN_H_

#include <types.h>

#define ENOCEAN_SENDER_ID_SIZE      0x04
#define ENOCEAN_MAX_DATA_SIZE       14

#define ENOCEAN_TELEGRAM_RORG_OFF   0x00
#define ENOCEAN_TELEGRAM_DATA_OFF   0x01

#define ENOCEAN_RSP_DATA_SIZE       0x01
#define ENOCEAN_1BS_DATA_SIZE       0x01

#define ENOCEAN_TELEGRAM_BUF_SIZE   21

/** Enocean Radio telegram types, 
 * see https://www.enocean-alliance.org/wp-content/uploads/2020/07/EnOcean-Equipment-Profiles-3-1.pdf
 */
typedef enum {
    RPS = 0xF6,
    BS_1 = 0xD5
} RORG; 

typedef unsigned char byte;

/**
 * @brief Structure containing the sender ID. Can be accessed as a 
 *          32 bit unsigned or byte per byte.
 */
typedef struct {
    union
    {
        uint32_t val;
        byte bytes[ENOCEAN_SENDER_ID_SIZE];
    };
    
} sender_id_t;

/**
 * @brief Enocean telegram struct
 * 
 * @param rorg Radio telegram type. See enum
 * @param data_len Byte contained in the data array
 * @param data Array containing telegram data
 * @param sender_id Enocean device unique ID
 * @param status Telegram status byte
 */
typedef struct {
    RORG rorg;
    int data_len;
    byte data[ENOCEAN_MAX_DATA_SIZE];
    sender_id_t sender_id;
    byte status;
} enocean_telegram_t;

/**
 * @brief Print an enocean telegram. Used for debug proposes
 * 
 * @param tel enocean telegram to print
 */
void enocean_print_telegram(enocean_telegram_t *tel);

/**
 * @brief Wait for data to be received by the enocean frontend. This function is blocking.
 * 
 * @return enocean_telegram_t* received enocean data
 */
enocean_telegram_t *enocean_get_data(void);

#endif
