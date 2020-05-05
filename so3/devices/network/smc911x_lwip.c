/*
 * Copyright (C) 2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <types.h>
#include <heap.h>
#include <vfs.h>

#include <process.h>
#include <signal.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/serial.h>
#include <device/irq.h>

#include <device/arch/pl011.h>

#include <mach/uart.h>

#include <asm/io.h>                 /* ioread/iowrite macros */

#include <printk.h>

// SPDX-License-Identifier: GPL-2.0+
/*
 * SMSC LAN9[12]1[567] Network driver
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 */

#include <common.h>
//#include <command.h>
//#include <malloc.h>
//#include <net.h>
//#include <miiphy.h>

//// !!!!!!!! FIX

#include <delay.h>
#include <device/network.h>

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
#include <lwip/tcpip.h>
#include <lwip/ip_addr.h>



#define ETHERNET_LAYER_2_MAX_LENGTH 1522

/* Basic mode control register. */
#define BMCR_RESV		0x003f	/* Unused...                   */
#define BMCR_SPEED1000		0x0040	/* MSB of Speed (1000)         */
#define BMCR_CTST		0x0080	/* Collision test              */
#define BMCR_FULLDPLX		0x0100	/* Full duplex                 */
#define BMCR_ANRESTART		0x0200	/* Auto negotiation restart    */
#define BMCR_ISOLATE		0x0400	/* Isolate data paths from MII */
#define BMCR_PDOWN		0x0800	/* Enable low power state      */
#define BMCR_ANENABLE		0x1000	/* Enable auto negotiation     */
#define BMCR_SPEED100		0x2000	/* Select 100Mbps              */
#define BMCR_LOOPBACK		0x4000	/* TXD loopback bits           */
#define BMCR_RESET		0x8000	/* Reset to default state      */
#define BMCR_SPEED10		0x0000	/* Select 10Mbps               */

/* Basic mode status register. */
#define BMSR_ERCAP		0x0001	/* Ext-reg capability          */
#define BMSR_JCD		0x0002	/* Jabber detected             */
#define BMSR_LSTATUS		0x0004	/* Link status                 */
#define BMSR_ANEGCAPABLE	0x0008	/* Able to do auto-negotiation */
#define BMSR_RFAULT		0x0010	/* Remote fault detected       */
#define BMSR_ANEGCOMPLETE	0x0020	/* Auto-negotiation complete   */
#define BMSR_RESV		0x00c0	/* Unused...                   */
#define BMSR_ESTATEN		0x0100	/* Extended Status in R15      */
#define BMSR_100HALF2		0x0200	/* Can do 100BASE-T2 HDX       */
#define BMSR_100FULL2		0x0400	/* Can do 100BASE-T2 FDX       */
#define BMSR_10HALF		0x0800	/* Can do 10mbps, half-duplex  */
#define BMSR_10FULL		0x1000	/* Can do 10mbps, full-duplex  */
#define BMSR_100HALF		0x2000	/* Can do 100mbps, half-duplex */
#define BMSR_100FULL		0x4000	/* Can do 100mbps, full-duplex */
#define BMSR_100BASE4		0x8000	/* Can do 100mbps, 4k packets  */



//struct eth_device *eth_dev;













//// !!!!!!!!!!!!!!!!!!


#include "smc911x.h"


u32 pkt_data_pull(eth_dev_t *dev, u32 addr) \
	__attribute__ ((weak, alias ("smc911x_reg_read")));
void pkt_data_push(eth_dev_t *dev, u32 addr, u32 val) \
	__attribute__ ((weak, alias ("smc911x_reg_write")));

static void smc911x_handle_mac_address(eth_dev_t *dev)
{
    unsigned long addrh, addrl;
    uchar *m = dev->enetaddr;

    addrl = m[0] | (m[1] << 8) | (m[2] << 16) | (m[3] << 24);
    addrh = m[4] | (m[5] << 8);
    smc911x_set_mac_csr(dev, ADDRL, addrl);
    smc911x_set_mac_csr(dev, ADDRH, addrh);

    printk(DRIVERNAME ": MAC %pM\n", m);
}

static int smc911x_eth_phy_read(eth_dev_t *dev,u8 phy, u8 reg, u16 *val)
{
    while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY);

    smc911x_set_mac_csr(dev, MII_ACC, phy << 11 | reg << 6 | MII_ACC_MII_BUSY);

    while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY);

    *val = smc911x_get_mac_csr(dev, MII_DATA);

    return 0;
}

static int smc911x_eth_phy_write(eth_dev_t *dev, u8 phy, u8 reg, u16  val)
{
    while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY);

    smc911x_set_mac_csr(dev, MII_DATA, val);
    smc911x_set_mac_csr(dev, MII_ACC, phy << 11 | reg << 6 | MII_ACC_MII_BUSY | MII_ACC_MII_WRITE);

    while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY);

    return 0;
}

static int smc911x_phy_reset(eth_dev_t *dev)
{
    u32 reg;

    reg = smc911x_reg_read(dev, PMT_CTRL);
    reg &= ~0xfffff030;
    reg |= PMT_CTRL_PHY_RST;
    smc911x_reg_write(dev, PMT_CTRL, reg);

    msleep(100);

   // udelay(100*1000);

    return 0;
}

static void smc911x_phy_configure(eth_dev_t *dev)
{
    int timeout;
    u16 status;

    smc911x_phy_reset(dev);

    smc911x_eth_phy_write(dev, 1, MII_BMCR, BMCR_RESET);
    udelay(1000);
    smc911x_eth_phy_write(dev, 1, MII_ADVERTISE, 0x01e1);
    smc911x_eth_phy_write(dev, 1, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);

    timeout = 5000;

    do {
        udelay(1000);
        if ((timeout--) == 0)
            goto err_out;

        if (smc911x_eth_phy_read(dev, 1, MII_BMSR, &status) != 0)
            goto err_out;

    } while (!(status & BMSR_LSTATUS));

    printk(DRIVERNAME ": phy initialized\n");

    return;

    err_out:
    printk(DRIVERNAME ": autonegotiation timed out\n");
}

static void smc911x_enable(eth_dev_t *dev)
{
    /* Enable TX */
    smc911x_reg_write(dev, HW_CFG, 8 << 16 | HW_CFG_SF);

    smc911x_reg_write(dev, GPT_CFG, GPT_CFG_TIMER_EN | 10000);

    smc911x_reg_write(dev, TX_CFG, TX_CFG_TX_ON);

    /* no padding to start of packets */
    smc911x_reg_write(dev, RX_CFG, 0);

    smc911x_set_mac_csr(dev, MAC_CR, MAC_CR_TXEN | MAC_CR_RXEN | MAC_CR_HBDIS);

}

static int smc911x_send(eth_dev_t *dev, void *packet, int length)
{
    u32 *data = (u32*)packet;
    u32 tmplen;
    u32 status;

    smc911x_reg_write(dev, TX_DATA_FIFO, TX_CMD_A_INT_FIRST_SEG |
                                         TX_CMD_A_INT_LAST_SEG | length);
    smc911x_reg_write(dev, TX_DATA_FIFO, length);

    tmplen = (length + 3) / 4;

    while (tmplen--)
    {
        pkt_data_push(dev, TX_DATA_FIFO, *data++);
    }

    /* wait for transmission */
    while (!((smc911x_reg_read(dev, TX_FIFO_INF) &
              TX_FIFO_INF_TSUSED) >> 16));

    /* get status. Ignore 'no carrier' error, it has no meaning for
     * full duplex operation
     */
    status = smc911x_reg_read(dev, TX_STATUS_FIFO) &
             (TX_STS_LOC | TX_STS_LATE_COLL | TX_STS_MANY_COLL |
              TX_STS_MANY_DEFER | TX_STS_UNDERRUN);

    if (!status)
        return 0;

    printk(DRIVERNAME ": failed to send packet: %s%s%s%s%s\n",
           status & TX_STS_LOC ? "TX_STS_LOC " : "",
           status & TX_STS_LATE_COLL ? "TX_STS_LATE_COLL " : "",
           status & TX_STS_MANY_COLL ? "TX_STS_MANY_COLL " : "",
           status & TX_STS_MANY_DEFER ? "TX_STS_MANY_DEFER " : "",
           status & TX_STS_UNDERRUN ? "TX_STS_UNDERRUN" : "");

    return -1;
}

static void smc911x_halt(eth_dev_t *dev)
{
    smc911x_reset(dev);
    smc911x_handle_mac_address(dev);
}


static int smc911x_rx(struct netif *netif)
{
    eth_dev_t *dev = netif->state;

    u32 *data = NULL;
    u32 pktlen, tmplen;
    u32 status, pulled_data;

    struct pbuf *buf;




    while ((smc911x_reg_read(dev, RX_FIFO_INF) & RX_FIFO_INF_RXSUSED) >> 16) {
        status = smc911x_reg_read(dev, RX_STATUS_FIFO);
        pktlen = (status & RX_STS_PKT_LEN) >> 16;

        if(status & RX_STS_ES){
            printk("dropped bad packet. Status: 0x%08x\n", status);
            return -1;
        }


        if(pktlen > ETHERNET_LAYER_2_MAX_LENGTH){
            printk("Dropped bad packet. Packet length can't exceed %d bytes, length was: %d bytes\n", ETHERNET_LAYER_2_MAX_LENGTH, pktlen);
            return -1;
        }


        smc911x_reg_write(dev, RX_CFG, 0);

        tmplen = (pktlen + 3) / 4;

        buf = pbuf_alloc(PBUF_RAW, pktlen, PBUF_RAM);

        if(buf != NULL) {
            data = (u32*)buf->payload;
        } else { // TODO better
            printk("No buff %d\n", pktlen);
            msleep(100);
            continue;
        }


        while (tmplen--)
        {
            pulled_data = pkt_data_pull(dev, RX_DATA_FIFO);

            if(data != NULL){
                *(data++) = pulled_data;
            }
        }


        if(buf != NULL)
            netif->input(buf, netif);
    }

    return 0;
}

static irq_return_t smc911x_so3_interrupt_top(int irq, void *dummy){
    struct netif* netif = (struct netif*)dummy;
    eth_dev_t *dev = netif->state;
    u32 mask;

    if(!sem_timeddown(&dev->sem_read, 10000)){

        // Disable frame interrupts
        mask = smc911x_reg_read(dev, INT_EN);
        smc911x_reg_write(dev, INT_EN, mask & ~(INT_EN_RSFL_EN | INT_EN_RSFF_EN));

        // Process incoming frames
        smc911x_rx(netif);

        // Re-enable frame interrupts
        smc911x_reg_write(dev, INT_EN, mask);
        sem_up(&dev->sem_read);

    }

    return IRQ_COMPLETED;
}

static irq_return_t smc911x_so3_interrupt(int irq, void *dummy)
{
    struct netif* netif = (struct netif*)dummy;
    eth_dev_t *dev = netif->state;

    irq_return_t irq_return = IRQ_COMPLETED;
    u32 status, mask, timeout;

    // Disable interrupts
    mask = smc911x_reg_read(dev, INT_EN);
    smc911x_reg_write(dev, INT_EN, 0);


    timeout = 8;

    do{

        status = smc911x_reg_read(dev, INT_STS);

        status &= mask;

        if(!status)
            break;

        if (status & INT_STS_SW_INT) {
            printk("STS_SW\n", status);
            smc911x_reg_write(dev, INT_STS, INT_STS_SW_INT);
        }
        /* Handle various error conditions */
        if (status & INT_STS_RXE) {
            printk("STS_RXE\n", status);
            smc911x_reg_write(dev, INT_STS, INT_STS_RXE);
        }
        if (status & INT_STS_RXDFH_INT) {
            printk("STS_RXDFH\n", status);
            smc911x_reg_write(dev, INT_STS, INT_STS_RXDFH_INT);
        }
        /* Undocumented interrupt-what is the right thing to do here? */
        if (status & INT_STS_RXDF_INT) {
            printk("STS_RXDF\n", status);
            smc911x_reg_write(dev, INT_STS, INT_STS_RXDF_INT);
        }
        /* Incoming frame */
        if (status & INT_STS_RSFL) {

            irq_return = IRQ_BOTTOM;
            smc911x_reg_write(dev, INT_STS, INT_STS_RSFL);
        }
        /* Rx Data FIFO exceeds set level */
        if (status & INT_STS_RDFL) {
            printk("STS_RDFL\n", status);
            smc911x_reg_write(dev, INT_STS, INT_STS_RDFL);
        }
        if (status & INT_STS_RDFO) {
            printk("STS_RDFO\n", status);
            smc911x_reg_write(dev, INT_STS, INT_STS_RDFO);
        }

        if(status & (INT_STS_TSFL | INT_STS_GPT_INT)){
            printk("STS_TSFL\n", status);
            smc911x_reg_write(dev, INT_STS, INT_STS_TSFL | INT_STS_GPT_INT);
        }

        if(status & INT_STS_PHY_INT){
            printk("PHY_INT\n", status);
            smc911x_reg_write(dev, INT_STS, INT_STS_PHY_INT);
        }

    } while(--timeout);

    smc911x_reg_write(dev, INT_STS, -1); // Clear all interrupts

    smc911x_reg_write(dev, INT_EN, mask); // Re-enable interrupts

    return irq_return;

}

static err_t smc911x_lwip_send(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;

    for (q = p; q != NULL; q = q->next) {

        // TODO FIX on saute 2 bytes au début car sinon que de zéros
        char* buff = (char*)q->payload;
        smc911x_send(netif->state, buff, q->len);
    }


    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);

    LINK_STATS_INC(link.xmit);

    return ERR_OK;
}

err_t smc911x_lwip_init(struct netif *netif)
{
    eth_dev_t *eth_dev = netif->state;
    struct chip_id *id = eth_dev->priv;
    u32 fifo;
    int i = 0;

    sem_init(&eth_dev->sem_read);

    LWIP_ASSERT("netif != NULL", (netif != NULL));

    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 1024*1024*10);

    netif->name[0] = 'e';
    netif->name[1] = 't';


    netif->hwaddr_len = ARP_HLEN;

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

#if LWIP_IPV4
    netif->output = etharp_output;
#endif

    netif->linkoutput = smc911x_lwip_send;



    printk(DRIVERNAME ": detected %s controller\n", id->name);

    smc911x_reset(eth_dev);

    /* Configure the PHY, initialize the link state */
    smc911x_phy_configure(eth_dev);

    smc911x_handle_mac_address(eth_dev);

    /* Turn on Tx + Rx */
    smc911x_enable(eth_dev);

    while(i < 6){
        netif->hwaddr[i] = eth_dev->enetaddr[i];
        i++;
    }


    // TODO change ??
    netif_set_default(netif);
    netif_set_link_up(netif);
    netif_set_up(netif);


    fifo = smc911x_reg_read(eth_dev, FIFO_INT);
    smc911x_reg_write(eth_dev, FIFO_INT, 0x01 | (fifo & 0xFFFFFF00));


    // Turn on relevant interrupts
    smc911x_reg_write(eth_dev, INT_EN, INT_EN_RSFL_EN | INT_EN_RSFF_EN);

    irq_bind(eth_dev->dev->irq, smc911x_so3_interrupt, smc911x_so3_interrupt_top, netif);

    // TODO remove ?
    dhcp_start(netif);

    return ERR_OK;
}

int smc911x_init(eth_dev_t * eth_dev)
{
    unsigned long addrl, addrh;
    struct netif* netif;


    if (!eth_dev) {
        return -1;
    }


    /* Try to detect chip. Will fail if not present. */
    if (smc911x_detect_chip(eth_dev)) {
        free(eth_dev);
        return 0;
    }

    addrh = smc911x_get_mac_csr(eth_dev, ADDRH);
    addrl = smc911x_get_mac_csr(eth_dev, ADDRL);
    if (!(addrl == 0xffffffff && addrh == 0x0000ffff)) {
        /* address is obtained from optional eeprom */
        eth_dev->enetaddr[0] = addrl;
        eth_dev->enetaddr[1] = addrl >>  8;
        eth_dev->enetaddr[2] = addrl >> 16;
        eth_dev->enetaddr[3] = addrl >> 24;
        eth_dev->enetaddr[4] = addrh;
        eth_dev->enetaddr[5] = addrh >> 8;
    }

    netif = malloc(sizeof(struct netif));
    netif_add(netif , NULL, NULL, NULL, eth_dev, smc911x_lwip_init, tcpip_input);


    return 1;
}



static int smc911x_register(dev_t *dev) {

    eth_dev_t *eth_dev = malloc(sizeof(eth_dev_t));
    memset(eth_dev, 0, sizeof(*eth_dev));


    eth_dev->dev = dev;
    eth_dev->iobase = eth_dev->dev->base;


    eth_dev->init = smc911x_init;
    network_devices_register(eth_dev);


    return 0;


}


REGISTER_DRIVER_POSTCORE("smsc,smc911x", smc911x_register);
