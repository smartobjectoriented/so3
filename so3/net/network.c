//
// Created by julien on 3/25/20.
//


#include <network.h>
#include <lwip/tcpip.h>
#include <device/network.h>





static void network_tcpip_done(void* args){
    // TODO init all network devices
    network_devices_init();
    printk("done");
}


void network_init(void){
    tcpip_init(network_tcpip_done, NULL);
}

