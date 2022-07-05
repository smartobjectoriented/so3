/*
 * Copyright (C) 2020 Julien Quartier <julien.quartier@heig-vd.ch>
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


#ifndef NETWORK_H
#define NETWORK_H

#include <types.h>
#include <list.h>
#include <device/device.h>
#include <semaphore.h>

#define ARP_HLEN 6

#define ETH_NAME_LEN 20

struct eth_dev {
    struct list_head list;

    char name[ETH_NAME_LEN];
    unsigned char enetaddr[ARP_HLEN];

    addr_t iobase;
    irq_def_t irq_def;

    int state;

    int (*init)(struct eth_dev *);
    int (*send)(struct eth_dev *, void *packet, int length);
    int (*recv)(struct eth_dev *);
    void (*halt)(struct eth_dev *);
    int (*write_hwaddr)(struct eth_dev *);
    void *priv;

    sem_t sem_read;
    sem_t sem_write;

    struct eth_dev *next;

};
typedef struct eth_dev eth_dev_t;

void network_devices_init(void);

void network_devices_register(eth_dev_t *eth_dev);

#endif /* NETWORK_H */
