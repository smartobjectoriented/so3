//
// Created by julien on 3/25/20.
//

#ifndef SO3__DEVICE_NETWORK_H
#define SO3__DEVICE_NETWORK_H

#include <types.h>
#include <list.h>
#include <device/device.h>
#include <semaphore.h>


#define ARP_HLEN 6


struct eth_dev {
    struct list_head list;
#define ETH_NAME_LEN 20
    char name[ETH_NAME_LEN];
    unsigned char enetaddr[ARP_HLEN];
    phys_addr_t iobase;
    int state;

    int (*init)(struct eth_dev *);
    int (*send)(struct eth_dev *, void *packet, int length);
    int (*recv)(struct eth_dev *);
    void (*halt)(struct eth_dev *);
    int (*write_hwaddr)(struct eth_dev *);
    void *priv;

    dev_t *dev;

    sem_t sem_read;
    sem_t sem_write;

    struct eth_dev *next;

};
typedef struct eth_dev eth_dev_t;

void network_devices_init(void);

void network_devices_register(eth_dev_t *eth_dev);

#endif //SO3__DEVICE_NETWORK_H
