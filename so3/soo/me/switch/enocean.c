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

#include <printk.h>
#include <heap.h>
#include <string.h>

#include <soo/enocean/enocean.h>
#include <soo/debug.h>
#include <soo/dev/venocean.h>

/**
 * @brief Convert a buffer containing EnOcean data to an enocean telegram structure.
 * 
 * @param buf Array of bytes containing the EnOcean data.
 * @param len Array length
 * @return enocean_telegram_t* Structure containing EnOcean data
 */
enocean_telegram_t * enocean_buffer_to_telegram(byte *buf, int len) {
    enocean_telegram_t *tel;
    int i;

    tel = (enocean_telegram_t*)malloc(sizeof(enocean_telegram_t));
    if (!tel)
        return NULL;
    
    switch(buf[ENOCEAN_TELEGRAM_RORG_OFF]) {
        case RPS:
            tel->rorg = RPS;
            tel->data_len = ENOCEAN_RSP_DATA_SIZE;
            break;
        case BS_1:
            tel->rorg = BS_1;
            tel->data_len = ENOCEAN_1BS_DATA_SIZE;
            break;
        default:
            DBG("Enocean telegram type 0x%02X is not yet supported\n", 
                buf[ENOCEAN_TELEGRAM_RORG_OFF]);
            return NULL;
    }

    memcpy(tel->data, &buf[ENOCEAN_TELEGRAM_DATA_OFF], tel->data_len);

    for (i = 0; i < ENOCEAN_SENDER_ID_SIZE; i++) {
        tel->sender_id.bytes[ENOCEAN_SENDER_ID_SIZE - (i + 1)] = 
                    buf[ENOCEAN_TELEGRAM_DATA_OFF + tel->data_len + i];
    }

    tel->status = buf[ENOCEAN_TELEGRAM_DATA_OFF + tel->data_len + ENOCEAN_SENDER_ID_SIZE];

    return tel;
}

enocean_telegram_t *enocean_get_data(void) {
    char data[ENOCEAN_TELEGRAM_BUF_SIZE];
    int data_len;
    
    data_len = venocean_get_data(data);

    return enocean_buffer_to_telegram(data, data_len);
}

void enocean_print_telegram(enocean_telegram_t *tel) {
    int i;

    printk("Enocean telegram:\n");
    printk("RORG: 0x%02X\n", tel->rorg);
    printk("Sender id: 0x%08X\n", tel->sender_id.val);
    printk("Data:\n");

    for (i = 0; i < tel->data_len; i++) {
        printk("[%d]: 0x%02X\n", i, tel->data[i]);
    }

    printk("Status: 0x%02X\n", tel->status);
}