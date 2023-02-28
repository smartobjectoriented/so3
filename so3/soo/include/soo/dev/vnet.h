/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef VNET_H
#define VNET_H

#include <device/network.h>
#include <soo/ring.h>
#include <soo/grant_table.h>
#include <soo/vdevfront.h>
#include <soo/dev/vnetbuff.h>


#define VNET_PACKET_SIZE	32

#define VNET_NAME		"vnet"
#define VNET_PREFIX		"[" VNET_NAME "] "



struct vnet_shared_data {
        unsigned char ethaddr[ARP_HLEN];
        uint32_t network;
        uint32_t mask;
        uint32_t me_ip;
        uint32_t agency_ip;
};


enum vnet_type{
        NET_STATUS = 0
};


struct ip_conf {
        uint32_t ip;
        uint32_t mask;
        uint32_t gw;
};

typedef struct {
        int broadcast_token;
        int connected;
} vnet_net_status_t;

typedef struct {
        uint16_t type;
        union {
                struct vbuff_data buff;
                vnet_net_status_t network;
        };
        char buffer[2];
} vnet_request_t;

typedef struct  {
        uint16_t type;
        union {
                struct vbuff_data buff;
                vnet_net_status_t network;
        };
        char buffer[2];
} vnet_response_t;

/*
 * Generate ring structures and types.
 */
DEFINE_RING_TYPES(vnet_data, vnet_request_t, vnet_response_t);
DEFINE_RING_TYPES(vnet_ctrl, vnet_request_t, vnet_response_t);


/*
 * General structure for this virtual device (backend side)
 */

typedef struct {
        vdevfront_t vdevfront;

        vnet_data_front_ring_t ring_data;
        vnet_ctrl_front_ring_t ring_ctrl;
        unsigned int irq;

        grant_ref_t ring_data_ref;
        grant_ref_t ring_ctrl_ref;
        grant_handle_t handle;
        uint32_t evtchn;

        int broadcast_token;
        int connected;

} vnet_t;

inline vnet_t *vnet_get_vnet(void);
inline struct vbuff_buff* vnet_get_vbuff_tx(void);
inline struct vbuff_buff* vnet_get_vbuff_rx(void);


static inline vnet_t *to_vnet(struct vbus_device *vdev) {
        vdevfront_t *vdevback = dev_get_drvdata(vdev->dev);
        return container_of(vdevback, vnet_t, vdevfront);
}
#endif /* VNET_H */
