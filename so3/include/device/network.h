//
// Created by julien on 3/25/20.
//

#ifndef SO3_NETWORK_H
#define SO3_NETWORK_H

#include <types.h>


#define ARP_HLEN 6


struct eth_device {
#define ETH_NAME_LEN 20
    char name[ETH_NAME_LEN];
    unsigned char enetaddr[ARP_HLEN];
    phys_addr_t iobase;
    int state;

    int (*init)(struct eth_device *);
    int (*send)(struct eth_device *, void *packet, int length);
    int (*recv)(struct eth_device *);
    void (*halt)(struct eth_device *);
    int (*mcast)(struct eth_device *, const u8 *enetaddr, int join);
    int (*write_hwaddr)(struct eth_device *);
    struct eth_device *next;
    int index;
    void *priv;
};


void network_init(void);

extern struct eth_device eth_dev;


#endif //SO3_NETWORK_H
