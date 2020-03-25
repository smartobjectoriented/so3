//
// Created by julien on 3/25/20.
//

#include <device/network.h>

struct eth_device eth_dev;

void network_init(void){
    eth_dev.init(&eth_dev);
}

