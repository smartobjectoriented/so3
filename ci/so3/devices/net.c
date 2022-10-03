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


#include <device/net.h>

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
