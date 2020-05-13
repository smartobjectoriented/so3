//
// Created by julien on 3/25/20.
//

#include <device/network.h>


struct list_head eth_dev_list = LIST_HEAD_INIT(eth_dev_list);

/*
 * Initialise all registered network devices.
 * LwIP MUST be fully initialised.
 */
void network_devices_init(void){
        eth_dev_t *eth_dev;

        list_for_each_entry(eth_dev, &eth_dev_list, list){
                if(eth_dev->init != NULL){
                        eth_dev->init(eth_dev);
                }
        }
}

/*
 * Add a device to the initialisation list.
 * The function init will be called when LwIP is fully initialised.
 */
void network_devices_register(eth_dev_t *eth_dev){
        list_add_tail(&eth_dev->list, &eth_dev_list);
}




