/*
 * Copyright (C) 2018-2019 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2018-2019 Baptiste Delporte <bonel@bonel.net>
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

#if 1
#define DEBUG
#endif

#include <heap.h>
#include <mutex.h>
#include <delay.h>
#include <memory.h>
#include <asm/mmu.h>

#include <device/driver.h>

#include <soo/evtchn.h>
#include <soo/gnttab.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/console.h>
#include <soo/debug.h>

#include <device/network.h>
#include <network.h>

#include <soo/dev/vnet.h>
#include <soo/dev/vnetbuff.h>

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include <lwip/init.h>
#include <lwip/dhcp.h>
#include <netif/ethernet.h>
#include <lwip/netifapi.h>
#include <lwip/tcpip.h>
#include <lwip/ip_addr.h>


/* Our unique net instance. */
static struct vbus_device *vdev_net = NULL;
vnet_t *vnet = NULL;

struct vbuff_buff vbuff_tx;
struct vbuff_buff vbuff_rx;
struct vnet_shared_data *vnet_shared_data;

grant_ref_t shared_data_grant = 0;

struct netif *netifg;

inline struct vbuff_buff* vnet_get_vbuff_tx(void){
        return &vbuff_tx;
}

inline struct vbuff_buff* vnet_get_vbuff_rx(void){
        return &vbuff_rx;
}

void vnet_ask_token(void) {
        vnet_request_t *ring_req;

        vnet->connected = 0;

        if((ring_req = vnet_ctrl_ring_request(&vnet->ring_ctrl)) != NULL) {
                ring_req->type = NET_STATUS;
                vnet_ctrl_ring_request_ready(&vnet->ring_ctrl);

                if(vnet->irq)
                        notify_remote_via_virq(vnet->irq);
        }
}


irq_return_t vnet_interrupt(int irq, void *dev_id) {
        struct vbus_device *vdev = (struct vbus_device *) dev_id;
        vnet_t *vnet = to_vnet(vdev);
        vnet_response_t *ring_rsp;
        struct pbuf *buf;
        unsigned char * data;

        DBG("%s, %d\n", __func__, ME_domID());

        while ((ring_rsp = vnet_ctrl_ring_response(&vnet->ring_ctrl)) != NULL) {
                switch(ring_rsp->type){
                case NET_STATUS:
                        vnet->broadcast_token = ring_rsp->network.broadcast_token;
                        vnet->connected = ring_rsp->network.connected;
                        printk("TOKEN %i, connected %i\n\n", vnet->broadcast_token, vnet->connected);
                        break;
                }
        }

        while ((ring_rsp = vnet_data_ring_response(&vnet->ring_data)) != NULL) {
                if((buf = pbuf_alloc(PBUF_RAW, ring_rsp->buff.size, PBUF_RAM)) != NULL){
                        data = vbuff_get(&vbuff_rx, &ring_rsp->buff);

                        memcpy(buf->payload, data, ring_rsp->buff.size);

                        netifg->input(buf, netifg);
                }
        }

        return IRQ_COMPLETED;
}

char vnet_buff[1514];
err_t vnet_lwip_send(struct netif *netif, struct pbuf *p) {
        vnet_t *vnet;
        char *data;
        void *buff = NULL;
        vnet_request_t *ring_req;
        struct vbuff_data vbuff_data;

        if (!vdev_net){
                goto send_failed;
        }

        vdevfront_processing_begin(vdev_net);

        vnet = to_vnet(vdev_net);

        vbuff_put(&vbuff_tx, &vbuff_data, &buff, p->tot_len);

        if(buff == NULL){
                goto send_failed;
        }

        data = (char *)pbuf_get_contiguous(p, buff, p->tot_len, p->tot_len, 0);

        /*
         * If only one pbuf is used get contiguous returns the pbuf pointer and don't use buff
         * We want to have the frame in buff in any case
         */
        if(data != buff){
                memcpy(buff, data, p->tot_len);
        }

        if((ring_req = vnet_data_ring_request(&vnet->ring_data)) == NULL) {
                goto send_failed;
        }
        ring_req->type = 0xefef;
        ring_req->buff = vbuff_data;

        vnet_data_ring_request_ready(&vnet->ring_data);

        if(vnet->irq)
                notify_remote_via_virq(vnet->irq);


        vdevfront_processing_end(vdev_net);
        return ERR_OK;

        send_failed:
        vdevfront_processing_end(vdev_net);
        return ERR_MEM;
}


void inline vnet_init_vnet(void){
        vnet_data_sring_t *sring_data;
        vnet_ctrl_sring_t *sring_ctrl;

        if(vnet != NULL)
                return;

        vnet = malloc(sizeof(vnet_t));
        BUG_ON(!vnet);
        memset(vnet, 0, sizeof(vnet_t));

        /* Prepare to set up the ring. */
        vnet->ring_data_ref = GRANT_INVALID_REF;
        vnet->ring_ctrl_ref = GRANT_INVALID_REF;

        /* Allocate a shared page for the ring */
        sring_data = (vnet_data_sring_t *) get_free_vpage();
        sring_ctrl = (vnet_ctrl_sring_t *) get_free_vpage();
        if (!sring_data || !sring_ctrl) {
                BUG();
        }

        SHARED_RING_INIT(sring_data);
        SHARED_RING_INIT(sring_ctrl);
        FRONT_RING_INIT(&vnet->ring_data, sring_data, PAGE_SIZE);
        FRONT_RING_INIT(&vnet->ring_ctrl, sring_ctrl, PAGE_SIZE);
}


inline vnet_t *vnet_get_vnet(void){
        vnet_init_vnet();

        return vnet;
}


void vnet_probe(struct vbus_device *vdev) {
        int res;
        unsigned int evtchn;

        struct vbus_transaction vbt;

        DBG0("[" VNET_NAME "] Frontend probe\n");

        if (vdev->state == VbusStateConnected)
                return ;

        /* Alloc a vnet struct if not already done */
        vnet_init_vnet();

        /* Local instance */
        vdev_net = vdev;

        dev_set_drvdata(vdev->dev, &vnet->vdevfront);

        DBG("Frontend: Setup ring\n");



        /* Allocate an event channel associated to the ring */
        res = vbus_alloc_evtchn(vdev, &evtchn);
        BUG_ON(res);

        res = bind_evtchn_to_irq_handler(evtchn, vnet_interrupt, NULL, vdev);
        if (res <= 0) {
                lprintk("%s - line %d: Binding event channel failed for device %s\n", __func__, __LINE__, vdev->nodename);
                BUG();
        }

        vnet->evtchn = evtchn;
        vnet->irq = res;



        /* Prepare the shared to page to be visible on the other end */
        res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((uint32_t) vnet->ring_data.sring)));
        if (res < 0)
                BUG();

        vnet->ring_data_ref = res;

        res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((uint32_t) vnet->ring_ctrl.sring)));
        if (res < 0)
                BUG();

        vnet->ring_ctrl_ref = res;


        /* Share a page containing infos about packet buffers */
        res = gnttab_grant_foreign_access(vdev->otherend_id, (uint32_t)phys_to_pfn(virt_to_phys_pt((uint32_t)vnet_shared_data)), !READ_ONLY);
        if (res < 0)
                BUG();

        shared_data_grant = res;
        vbuff_update_grant(&vbuff_tx, vdev);
        vbuff_update_grant(&vbuff_rx, vdev);


        vbus_transaction_start(&vbt);

        vbus_printf(vbt, vdev->nodename, "ring-data-ref", "%u", vnet->ring_data_ref);
        vbus_printf(vbt, vdev->nodename, "ring-ctrl-ref", "%u", vnet->ring_ctrl_ref);
        vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vnet->evtchn);
        vbus_printf(vbt, vdev->nodename, "grant-buff", "%u", shared_data_grant);
        vbus_printf(vbt, vdev->nodename, "vbuff-tx-ref", "%u", vbuff_tx.grants[0]); // Only the first grant is needed to map memory
        vbus_printf(vbt, vdev->nodename, "vbuff-rx-ref", "%u", vbuff_rx.grants[0]);

        vbus_transaction_end(vbt);
}

/* At this point, the FE is not connected. */
void vnet_reconfiguring(struct vbus_device *vdev) {
        int res;
        struct vbus_transaction vbt;
        vnet_t *vnet = to_vnet(vdev);

        DBG0("[" VNET_NAME "] Frontend reconfiguring\n");
        /* The shared page already exists */
        /* Re-init */

        gnttab_end_foreign_access_ref(vnet->ring_data_ref);
        gnttab_end_foreign_access_ref(vnet->ring_ctrl_ref);

        DBG("Frontend: Setup ring\n");

        /* Prepare to set up the ring. */

        vnet->ring_data_ref = GRANT_INVALID_REF;
        vnet->ring_ctrl_ref = GRANT_INVALID_REF;

        SHARED_RING_INIT(vnet->ring_data.sring);
        SHARED_RING_INIT(vnet->ring_ctrl.sring);
        FRONT_RING_INIT(&vnet->ring_data, (&vnet->ring_data)->sring, PAGE_SIZE);
        FRONT_RING_INIT(&vnet->ring_ctrl, (&vnet->ring_ctrl)->sring, PAGE_SIZE);


        /* Prepare the shared to page to be visible on the other end */

        res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((uint32_t) vnet->ring_data.sring)));
        if (res < 0)
                BUG();

        vnet->ring_data_ref = res;


        res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((uint32_t) vnet->ring_ctrl.sring)));
        if (res < 0)
                BUG();

        vnet->ring_ctrl_ref = res;



        /* Share a page containing shared data */
        res = gnttab_grant_foreign_access(vdev->otherend_id, (unsigned long)phys_to_pfn(virt_to_phys_pt((uint32_t)vnet_shared_data)), !READ_ONLY);
        if (res < 0)
                BUG();

        shared_data_grant = res;
        vbuff_update_grant(&vbuff_tx, vdev);
        vbuff_update_grant(&vbuff_rx, vdev);


        vbus_transaction_start(&vbt);

        vbus_printf(vbt, vdev->nodename, "ring-data-ref", "%u", vnet->ring_data_ref);
        vbus_printf(vbt, vdev->nodename, "ring-ctrl-ref", "%u", vnet->ring_ctrl_ref);
        vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vnet->evtchn);
        vbus_printf(vbt, vdev->nodename, "grant-buff", "%u", shared_data_grant);
        vbus_printf(vbt, vdev->nodename, "vbuff-tx-ref", "%u", vbuff_tx.grants[0]); // Only the first grant is needed to map memory
        vbus_printf(vbt, vdev->nodename, "vbuff-rx-ref", "%u", vbuff_rx.grants[0]);


        vbus_transaction_end(vbt);
}

void vnet_shutdown(struct vbus_device *vdev) {
        DBG0("[" VNET_NAME "] Frontend shutdown\n");
}

void vnet_closed(struct vbus_device *vdev) {
        vnet_t *vnet = to_vnet(vdev);

        DBG0("[" VNET_NAME "] Frontend close\n");

        /* Free packet buffers */
        vbuff_free(&vbuff_tx);
        vbuff_free(&vbuff_rx);

        /**
         * Free the ring and deallocate the proper data.
         */

        /* Free resources associated with old device channel. */
        if (vnet->ring_data_ref != GRANT_INVALID_REF) {
                gnttab_end_foreign_access(vnet->ring_data_ref);
                free_vpage((uint32_t) vnet->ring_data.sring);

                vnet->ring_data_ref = GRANT_INVALID_REF;
                vnet->ring_data.sring = NULL;
        }

        if (vnet->ring_ctrl_ref != GRANT_INVALID_REF) {
                gnttab_end_foreign_access(vnet->ring_ctrl_ref);
                free_vpage((uint32_t) vnet->ring_ctrl.sring);

                vnet->ring_ctrl_ref = GRANT_INVALID_REF;
                vnet->ring_ctrl.sring = NULL;
        }

        if (vnet->irq)
                unbind_from_irqhandler(vnet->irq);

        vnet->irq = 0;
}

void vnet_suspend(struct vbus_device *vdev) {

        DBG0("[" VNET_NAME "] Frontend suspend\n");
}

void vnet_resume(struct vbus_device *vdev) {

        DBG0("[" VNET_NAME "] Frontend resume\n");
}

void vnet_connected(struct vbus_device *vdev) {
        vnet_t *vnet = to_vnet(vdev);

        DBG0("[" VNET_NAME "] Frontend connected\n");

        /* Force the processing of pending requests, if any */
        if(vnet->irq)
                notify_remote_via_virq(vnet->irq);

        vnet_ask_token();
}


err_t vnet_lwip_init(struct netif *netif) {
        ip4_addr_t ip, mask, gw;
        eth_dev_t *eth_dev = netif->state;
        LWIP_ASSERT("netif != NULL", (netif != NULL));
        netif->name[0] = 'v';
        netif->name[1] = 'i';
        printk("vnet_lwip_init\n");
        netif->hwaddr_len = ARP_HLEN;
        netif->mtu = 1500;
        netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;


#if LWIP_IPV4
        netif->output = etharp_output;
#endif

        netif->linkoutput = vnet_lwip_send;

        memcpy(netif->hwaddr, eth_dev->enetaddr, ARP_HLEN);

        netifg = netif;

#ifdef CONFIG_VNET_FRONT_DHCP
        dhcp_start(netif);
#else
        ip.addr = vnet_shared_data->me_ip;
        mask.addr = vnet_shared_data->mask;
        gw.addr = vnet_shared_data->agency_ip;

        netif_set_addr(netif, &ip, &mask, &gw);
#endif /* CONFIG_VNET_FRONT_DHCP */

        netif_set_default(netif);
        netif_set_link_up(netif);
        netif_set_up(netif);

        return ERR_OK;
}

int vnet_init(eth_dev_t *eth_dev) {
        struct netif *netif;
        printk("vnet_init\n");

        memcpy(eth_dev->enetaddr, vnet_shared_data->ethaddr, ARP_HLEN);

        netif = malloc(sizeof(struct netif));
        netif_add(netif, NULL, NULL, NULL, eth_dev, vnet_lwip_init, tcpip_input);

        return 1;
}

vdrvfront_t vnetdrv = {
        .probe = vnet_probe,
        .reconfiguring = vnet_reconfiguring,
        .shutdown = vnet_shutdown,
        .closed = vnet_closed,
        .suspend = vnet_suspend,
        .resume = vnet_resume,
        .connected = vnet_connected
};


static void vnet_generate_network(void){
        ME_desc_t *desc = get_ME_desc();
        unsigned int crc32 = desc->crc32;
        uint32_t network, me_ip, mask, tmp;

        network = 0x00000cc0au; /* 10.204.x.x */
        network |= (crc32 << 16) ^ (crc32 & 0xffff0000u);

#ifdef CONFIG_VNET_BACKEND_ARCH_NAT
        /* Create a network with only two host addresses */
        mask = 0xfcffffffu; /* 255.255.255.252 */
        me_ip = network & mask | 2u << 24; /* the ME is always the second host IP */
#else
        /* a big shared network between all MEs */
        mask = 0x0000ffffu; /* 255.255.0.0 */

        tmp = (network & mask) ^ network;

        /* check if the ip is not the network address nor the gateway address */
        if(tmp != 0 && tmp != 0x01 << 24)
                me_ip = network;
        else
                me_ip = network | 0x02 << 24;
#endif


        vnet_shared_data->network = network & mask;
        vnet_shared_data->me_ip = me_ip;
        vnet_shared_data->agency_ip = vnet_shared_data->network | 1u << 24;
        vnet_shared_data->mask = mask;
}


static void vnet_generate_mac(void){
        ME_desc_t *desc = get_ME_desc();
        unsigned int crc32 = desc->crc32;

        vnet_shared_data->ethaddr[0] = 0xde;
        vnet_shared_data->ethaddr[1] = 0xad;
        vnet_shared_data->ethaddr[2] = 0xbe;
        vnet_shared_data->ethaddr[3] = (crc32 ^ (crc32 >> 8)) & 0xFF;
        vnet_shared_data->ethaddr[4] = (crc32 >> 16) & 0xFF;
        vnet_shared_data->ethaddr[5] = (crc32 >> 24) & 0xFF;
}


static int vnet_register(dev_t *dev) {
        eth_dev_t *eth_dev = malloc(sizeof(eth_dev_t));
        memset(eth_dev, 0, sizeof(*eth_dev));

        vdevfront_init(VNET_NAME, &vnetdrv);

        eth_dev->dev = dev;

        eth_dev->init = vnet_init;
        network_devices_register(eth_dev);

        /* alloc page for buffer status */
        vnet_shared_data = (struct vnet_shared_data *)get_free_vpage();

        /* init packet buffers */
        vbuff_init(&vbuff_tx);
        vbuff_init(&vbuff_rx);

        vnet_generate_mac();
        vnet_generate_network();



        return 0;
}

REGISTER_DRIVER_POSTCORE("vnet,frontend", vnet_register);
