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

#define ETHERNET_LAYER_2_MAX_LENGTH 1522

/* Generic MII registers. */
#define MII_BMCR                0x00    /* Basic mode control register */
#define MII_BMSR                0x01    /* Basic mode status register  */
#define MII_PHYSID1             0x02    /* PHYS ID 1                   */
#define MII_PHYSID2             0x03    /* PHYS ID 2                   */
#define MII_ADVERTISE           0x04    /* Advertisement control reg   */
#define MII_LPA                 0x05    /* Link partner ability reg    */
#define MII_EXPANSION           0x06    /* Expansion register          */
#define MII_CTRL1000            0x09    /* 1000BASE-T control          */
#define MII_STAT1000            0x0a    /* 1000BASE-T status           */
#define MII_MMD_CTRL            0x0d    /* MMD Access Control Register */
#define MII_MMD_DATA            0x0e    /* MMD Access Data Register */
#define MII_ESTATUS             0x0f    /* Extended Status             */
#define MII_DCOUNTER            0x12    /* Disconnect counter          */
#define MII_FCSCOUNTER          0x13    /* False carrier counter       */
#define MII_NWAYTEST            0x14    /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER         0x15    /* Receive error counter       */
#define MII_SREVISION           0x16    /* Silicon revision            */
#define MII_RESV1               0x17    /* Reserved...                 */
#define MII_LBRERROR            0x18    /* Lpback, rx, bypass error    */
#define MII_PHYADDR             0x19    /* PHY address                 */
#define MII_RESV2               0x1a    /* Reserved...                 */
#define MII_TPISTATUS           0x1b    /* TPI status for 10mbps       */
#define MII_NCONFIG             0x1c    /* Network interface config    */

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


u32 **net_rx_packets;

struct eth_device *eth_dev;













//// !!!!!!!!!!!!!!!!!!


#include "smc911x.h"




u32 pkt_data_pull(struct eth_device *dev, u32 addr) \
	__attribute__ ((weak, alias ("smc911x_reg_read")));
void pkt_data_push(struct eth_device *dev, u32 addr, u32 val) \
	__attribute__ ((weak, alias ("smc911x_reg_write")));

static void smc911x_handle_mac_address(struct eth_device *dev)
{
    unsigned long addrh, addrl;
    uchar *m = dev->enetaddr;

    addrl = m[0] | (m[1] << 8) | (m[2] << 16) | (m[3] << 24);
    addrh = m[4] | (m[5] << 8);
    smc911x_set_mac_csr(dev, ADDRL, addrl);
    smc911x_set_mac_csr(dev, ADDRH, addrh);

    printk(DRIVERNAME ": MAC %pM\n", m);
}

static int smc911x_eth_phy_read(struct eth_device *dev,
                                u8 phy, u8 reg, u16 *val)
{
    while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY)
        ;

    smc911x_set_mac_csr(dev, MII_ACC, phy << 11 | reg << 6 |
                                      MII_ACC_MII_BUSY);

    while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY)
        ;

    *val = smc911x_get_mac_csr(dev, MII_DATA);

    return 0;
}

static int smc911x_eth_phy_write(struct eth_device *dev,
                                 u8 phy, u8 reg, u16  val)
{
    while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY)
        ;

    smc911x_set_mac_csr(dev, MII_DATA, val);
    smc911x_set_mac_csr(dev, MII_ACC,
                        phy << 11 | reg << 6 | MII_ACC_MII_BUSY | MII_ACC_MII_WRITE);

    while (smc911x_get_mac_csr(dev, MII_ACC) & MII_ACC_MII_BUSY)
        ;
    return 0;
}

static int smc911x_phy_reset(struct eth_device *dev)
{
    u32 reg;

    reg = smc911x_reg_read(dev, PMT_CTRL);
    reg &= ~0xfffff030;
    reg |= PMT_CTRL_PHY_RST;
    smc911x_reg_write(dev, PMT_CTRL, reg);

    msleep(100);

    return 0;
}

static void smc911x_phy_configure(struct eth_device *dev)
{
    int timeout;
    u16 status;

    smc911x_phy_reset(dev);

    smc911x_eth_phy_write(dev, 1, MII_BMCR, BMCR_RESET);
    udelay(1000);
    smc911x_eth_phy_write(dev, 1, MII_ADVERTISE, 0x01e1);
    smc911x_eth_phy_write(dev, 1, MII_BMCR, BMCR_ANENABLE |
                                            BMCR_ANRESTART);

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

static void smc911x_enable(struct eth_device *dev)
{
    /* Enable TX */
    smc911x_reg_write(dev, HW_CFG, 8 << 16 | HW_CFG_SF);

    smc911x_reg_write(dev, GPT_CFG, GPT_CFG_TIMER_EN | 10000);

    smc911x_reg_write(dev, TX_CFG, TX_CFG_TX_ON);

    /* no padding to start of packets */
    smc911x_reg_write(dev, RX_CFG, 0);

    smc911x_set_mac_csr(dev, MAC_CR, MAC_CR_TXEN | MAC_CR_RXEN |
                                     MAC_CR_HBDIS);

}

static int smc911x_init(struct eth_device *dev)
{
    struct chip_id *id = dev->priv;

    printk(DRIVERNAME ": detected %s controller\n", id->name);

    smc911x_reset(dev);

    /* Configure the PHY, initialize the link state */
    smc911x_phy_configure(dev);

    smc911x_handle_mac_address(dev);

    /* Turn on Tx + Rx */
    smc911x_enable(dev);

    return 0;
}

static int smc911x_send(struct eth_device *dev, void *packet, int length)
{
    u32 *data = (u32*)packet;
    u32 tmplen;
    u32 status;

    smc911x_reg_write(dev, TX_DATA_FIFO, TX_CMD_A_INT_FIRST_SEG |
                                         TX_CMD_A_INT_LAST_SEG | length);
    smc911x_reg_write(dev, TX_DATA_FIFO, length);

    tmplen = (length + 3) / 4;

    while (tmplen--)
        pkt_data_push(dev, TX_DATA_FIFO, *data++);

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

static void smc911x_halt(struct eth_device *dev)
{
    smc911x_reset(dev);
    smc911x_handle_mac_address(dev);
}


static int smc911x_rx(struct eth_device *dev)
{
    static int i = 0;
    //printk("RX1 %d\n", i++);
    u32 *data = (u32 *)net_rx_packets;
    u32 pktlen, tmplen;
    u32 status;

    if ((smc911x_reg_read(dev, RX_FIFO_INF) & RX_FIFO_INF_RXSUSED) >> 16) {
        status = smc911x_reg_read(dev, RX_STATUS_FIFO);
        pktlen = (status & RX_STS_PKT_LEN) >> 16;

        if(status & RX_STS_ES){
            printk("dropped bad packet. Status: 0x%08x\n", status);
            return 0;
        }


        if(pktlen > ETHERNET_LAYER_2_MAX_LENGTH){
            printk("Dropped bad packet. Packet length can't exceed %d bytes, length was: %d bytes\n", ETHERNET_LAYER_2_MAX_LENGTH, pktlen);
            return 0;
        }

        //printk("Len %d\n", pktlen);


        smc911x_reg_write(dev, RX_CFG, 0);

        tmplen = (pktlen + 3) / 4;

        while (tmplen--)
            *(data++) = pkt_data_pull(dev, RX_DATA_FIFO);


        // TODO handle packet

        printk("\npkt\n\n");

        /*if (status & RX_STS_ES)
            printk(DRIVERNAME
                   ": dropped bad packet. Status: 0x%08x\n",
                   status);
        else
            net_process_received_packet(net_rx_packets[0], pktlen);*/
    }

    //printk("Received");

    return 0;
}

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
/* wrapper for smc911x_eth_phy_read */
static int smc911x_miiphy_read(struct mii_dev *bus, int phy, int devad,
			       int reg)
{
	u16 val = 0;
	struct eth_device *dev = eth_get_dev_by_name(bus->name);
	if (dev) {
		int retval = smc911x_eth_phy_read(dev, phy, reg, &val);
		if (retval < 0)
			return retval;
		return val;
	}
	return -ENODEV;
}
/* wrapper for smc911x_eth_phy_write */
static int smc911x_miiphy_write(struct mii_dev *bus, int phy, int devad,
				int reg, u16 val)
{
	struct eth_device *dev = eth_get_dev_by_name(bus->name);
	if (dev)
		return smc911x_eth_phy_write(dev, phy, reg, val);
	return -ENODEV;
}
#endif


int smc911x_initialize(u8 dev_num, int base_addr, struct eth_device * dev)
{
    unsigned long addrl, addrh;
    //struct eth_device *dev;


    if (!dev) {
        return -1;
    }
    memset(dev, 0, sizeof(*dev));

    dev->iobase = base_addr;

    /* Try to detect chip. Will fail if not present. */
    if (smc911x_detect_chip(dev)) {
        free(dev);
        return 0;
    }

    addrh = smc911x_get_mac_csr(dev, ADDRH);
    addrl = smc911x_get_mac_csr(dev, ADDRL);
    if (!(addrl == 0xffffffff && addrh == 0x0000ffff)) {
        /* address is obtained from optional eeprom */
        dev->enetaddr[0] = addrl;
        dev->enetaddr[1] = addrl >>  8;
        dev->enetaddr[2] = addrl >> 16;
        dev->enetaddr[3] = addrl >> 24;
        dev->enetaddr[4] = addrh;
        dev->enetaddr[5] = addrh >> 8;
    }

    dev->init = smc911x_init;
    dev->halt = smc911x_halt;
    dev->send = smc911x_send;
    dev->recv = smc911x_rx;
    //dev->name = "test_eth";
    //sprintk(dev->name, "%s-%hu", DRIVERNAME, dev_num);

    //eth_register(dev);

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
    int retval;
	struct mii_dev *mdiodev = mdio_alloc();
	if (!mdiodev)
		return -ENOMEM;
	strncpy(mdiodev->name, dev->name, MDIO_NAME_LEN);
	mdiodev->read = smc911x_miiphy_read;
	mdiodev->write = smc911x_miiphy_write;

	retval = mdio_register(mdiodev);
	if (retval < 0)
		return retval;
#endif

    return 1;
}


static irq_return_t smc91x_int(int irq, void *dummy)
{
    smc911x_rx(dummy);
}

static int smc91x_init(dev_t *dev) {

    net_rx_packets = malloc(ETHERNET_LAYER_2_MAX_LENGTH);

    eth_dev = malloc(sizeof(*eth_dev));

    printk("----- START NET -----\n");

    if(smc911x_initialize(0, dev->base, eth_dev)){


        irq_bind(dev->irq, smc91x_int, NULL, eth_dev);

        eth_dev->init(eth_dev);

        //46:d4:cf:19:a3:01

        // mac sender
        /*uchar *ms = eth_dev->enetaddr;

        printk("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",ms[0],ms[1],ms[2],ms[3],ms[4],ms[5]);

        // mac dest
        uchar md[6] = {0x46, 0xd4, 0xcf, 0x19, 0xa3, 0x01};


        u32 b1 = md[0] | (md[1] << 8) | (md[2] << 16) | (md[3] << 24);
        u32 b2 = (md[4]) | (md[5] << 8) | (ms[0] << 16) | (ms[1] << 24);
        u32 b3 = (ms[2]) | (ms[3] << 8) | (ms[4] << 16) | (ms[5] << 24);

        u32 data[100] = {b1,b2,b3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10};

        int i = 0;


        while(i++ < 40){
            eth_dev->send(eth_dev, data,100);
            printk("Send %d", i);
            msleep(5000);
        }*/

    }

    printk("----- END NET -----\n");

    return 0;
}

/*
err_t myif_init(struct netif *netif){

}*/


//REGISTER_DRIVER_POSTCORE("smsc,smc911x", smc91x_init);
