/*
 * Copyright (C) 2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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
 * vnet frontend: the packets of the capsule which carries a network, to and
 * from its agency (soo/dev/vnet.h). The capsule reads and writes them on
 * /dev/vnet, each preceded by its port.
 */

#if 0
#define DEBUG
#endif

#include <heap.h>
#include <memory.h>

#include <asm/mmu.h>

#include <device/driver.h>

#include <soo/evtchn.h>
#include <soo/gnttab.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/debug.h>

#include <soo/dev/vnet.h>

typedef struct {
	/* Must be the first field */
	vnet_t vnet;

	/* Readers wait for a packet, writers for a free slot. */

	completion_t rx_wait;
	completion_t tx_wait;

} vnet_priv_t;

static struct vbus_device *vnet_dev = NULL;

/**
 * The agency put packets in the ring, or freed slots: wake who waits.
 *
 * @param irq		the interrupt
 * @param dev_id	the vbus device
 * @return IRQ_COMPLETED
 */

static irq_return_t vnet_interrupt(int irq, void *dev_id)
{
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	vnet_priv_t *vnet_priv = (vnet_priv_t *) dev_get_drvdata(vdev->dev);

	complete(&vnet_priv->rx_wait);
	complete(&vnet_priv->tx_wait);

	return IRQ_COMPLETED;
}

/**
 * Grant the ring and the data pages to the agency, and tell it where they are.
 *
 * @param vdev		the vbus device
 * @param vnet_priv	the device's state
 */

static void vnet_grant(struct vbus_device *vdev, vnet_priv_t *vnet_priv)
{
	struct vbus_transaction vbt;
	char key[16];
	int i;

	vnet_priv->vnet.ring_ref = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vnet_priv->vnet.ring.sring)));

	for (i = 0; i < VNET_PAGES; i++) {
		vnet_priv->vnet.tx_ref[i] = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt(vnet_priv->vnet.tx_page[i])));
		vnet_priv->vnet.rx_ref[i] = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt(vnet_priv->vnet.rx_page[i])));
	}

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vnet_priv->vnet.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vnet_priv->vnet.evtchn);

	for (i = 0; i < VNET_PAGES; i++) {
		sprintf(key, "tx-ref-%d", i);
		vbus_printf(vbt, vdev->nodename, key, "%u", vnet_priv->vnet.tx_ref[i]);
		sprintf(key, "rx-ref-%d", i);
		vbus_printf(vbt, vdev->nodename, key, "%u", vnet_priv->vnet.rx_ref[i]);
	}

	vbus_transaction_end(vbt);
}

/**
 * Take back what was granted to the agency.
 *
 * @param vnet_priv	the device's state
 */

static void vnet_ungrant(vnet_priv_t *vnet_priv)
{
	int i;

	if (vnet_priv->vnet.ring_ref == GRANT_INVALID_REF)
		return;

	gnttab_end_foreign_access(vnet_priv->vnet.ring_ref);

	for (i = 0; i < VNET_PAGES; i++) {
		gnttab_end_foreign_access(vnet_priv->vnet.tx_ref[i]);
		gnttab_end_foreign_access(vnet_priv->vnet.rx_ref[i]);
	}

	vnet_priv->vnet.ring_ref = GRANT_INVALID_REF;
}

/**
 * Set the device up: its event channel, its ring and its data pages.
 *
 * @param vdev	the vbus device
 */

static void vnet_probe(struct vbus_device *vdev)
{
	vnet_priv_t *vnet_priv;
	vnet_sring_t *sring;
	unsigned int evtchn;
	int i;

	DBG0(VNET_PREFIX "Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return;

	vnet_priv = dev_get_drvdata(vdev->dev);
	vnet_dev = vdev;

	init_completion(&vnet_priv->rx_wait);
	init_completion(&vnet_priv->tx_wait);

	vbus_alloc_evtchn(vdev, &evtchn);
	vnet_priv->vnet.irq = bind_evtchn_to_irq_handler(evtchn, vnet_interrupt, NULL, vdev);
	vnet_priv->vnet.evtchn = evtchn;

	sring = (vnet_sring_t *) get_free_vpage();
	BUG_ON(!sring);

	for (i = 0; i < VNET_PAGES; i++) {
		vnet_priv->vnet.tx_page[i] = get_free_vpage();
		vnet_priv->vnet.rx_page[i] = get_free_vpage();
		BUG_ON(!vnet_priv->vnet.tx_page[i] || !vnet_priv->vnet.rx_page[i]);
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&vnet_priv->vnet.ring, sring, PAGE_SIZE);

	vnet_grant(vdev, vnet_priv);
}

/**
 * Set the ring up again, on a new device: the packets in flight are lost.
 *
 * @param vdev	the vbus device
 */

static void vnet_reconfiguring(struct vbus_device *vdev)
{
	vnet_priv_t *vnet_priv = dev_get_drvdata(vdev->dev);

	DBG0(VNET_PREFIX "Frontend reconfiguring\n");

	vnet_ungrant(vnet_priv);

	SHARED_RING_INIT(vnet_priv->vnet.ring.sring);
	FRONT_RING_INIT(&vnet_priv->vnet.ring, vnet_priv->vnet.ring.sring, PAGE_SIZE);

	vnet_grant(vdev, vnet_priv);
}

/**
 * Free what the device holds: the agency is gone.
 *
 * @param vdev	the vbus device
 */

static void vnet_closed(struct vbus_device *vdev)
{
	vnet_priv_t *vnet_priv = dev_get_drvdata(vdev->dev);
	int i;

	DBG0(VNET_PREFIX "Frontend close\n");

	vnet_ungrant(vnet_priv);

	if (vnet_priv->vnet.ring.sring) {
		free_vpage((addr_t) vnet_priv->vnet.ring.sring);
		vnet_priv->vnet.ring.sring = NULL;

		for (i = 0; i < VNET_PAGES; i++) {
			free_vpage(vnet_priv->vnet.tx_page[i]);
			free_vpage(vnet_priv->vnet.rx_page[i]);
		}
	}

	if (vnet_priv->vnet.irq)
		unbind_from_irqhandler(vnet_priv->vnet.irq);

	vnet_priv->vnet.irq = 0;
}

/**
 * Nothing to do on these steps of the device's life.
 *
 * @param vdev	the vbus device
 */

static void vnet_nothing(struct vbus_device *vdev)
{
}

vdrvfront_t vnetdrv = { .probe = vnet_probe,
			.reconfiguring = vnet_reconfiguring,
			.shutdown = vnet_nothing,
			.closed = vnet_closed,
			.suspend = vnet_nothing,
			.resume = vnet_nothing,
			.connected = vnet_nothing };

/**
 * Read the next packet from the agency, waiting for one: its port, then its
 * data. The slot is released only once copied, so the agency cannot reuse
 * it meanwhile; the agency hears of it if the ring was full, as packets may
 * wait for room on its side.
 *
 * @param fd		unused
 * @param buffer	a struct vnet_packet
 * @param count		its size
 * @return the length read, port included, or -1
 */

static int vnet_cread(int fd, void *buffer, int count)
{
	struct vnet_packet *p = buffer;
	vnet_priv_t *vnet_priv;
	vnet_response_t *rsp;
	vnet_sring_t *sring;
	RING_IDX idx;
	bool full;
	int len;

	if (!vnet_dev || count < (int) sizeof(p->port))
		return -1;

	vnet_priv = dev_get_drvdata(vnet_dev->dev);

	for (;;) {
		vdevfront_processing_begin(vnet_dev);

		sring = vnet_priv->vnet.ring.sring;
		if (sring->rsp_cons != sring->rsp_prod)
			break;

		vdevfront_processing_end(vnet_dev);
		wait_for_completion(&vnet_priv->rx_wait);
	}

	rmb();

	idx = sring->rsp_cons;
	full = sring->rsp_prod - idx >= VNET_SLOTS;
	rsp = RING_GET_RESPONSE(&vnet_priv->vnet.ring, idx);

	len = rsp->len;
	if (len > VNET_MTU || len > count - (int) sizeof(p->port))
		len = 0;

	p->port = rsp->port;
	memcpy(p->data, VNET_SLOT(vnet_priv->vnet.rx_page, idx), len);

	mb();
	sring->rsp_cons = idx + 1;

	if (full)
		notify_remote_via_virq(vnet_priv->vnet.irq);

	vdevfront_processing_end(vnet_dev);

	return sizeof(p->port) + len;
}

/**
 * Write a packet to the agency, waiting for a free slot: its port, then its
 * data.
 *
 * @param fd		unused
 * @param buffer	a struct vnet_packet
 * @param count		the port and the packet's length
 * @return count, or -1
 */

static int vnet_cwrite(int fd, const void *buffer, int count)
{
	const struct vnet_packet *p = buffer;
	vnet_priv_t *vnet_priv;
	vnet_request_t *req;
	vnet_front_ring_t *ring;
	int len = count - (int) sizeof(p->port);

	if (!vnet_dev || len <= 0 || len > VNET_MTU)
		return -1;

	vnet_priv = dev_get_drvdata(vnet_dev->dev);
	ring = &vnet_priv->vnet.ring;

	for (;;) {
		vdevfront_processing_begin(vnet_dev);

		if (ring->req_prod_pvt - ring->sring->req_cons < VNET_SLOTS)
			break;

		vdevfront_processing_end(vnet_dev);
		wait_for_completion(&vnet_priv->tx_wait);
	}

	memcpy(VNET_SLOT(vnet_priv->vnet.tx_page, ring->req_prod_pvt), p->data, len);

	req = RING_GET_REQUEST(ring, ring->req_prod_pvt);
	req->port = p->port;
	req->len = len;
	ring->req_prod_pvt++;

	RING_PUSH_REQUESTS(ring);
	notify_remote_via_virq(vnet_priv->vnet.irq);

	vdevfront_processing_end(vnet_dev);

	return count;
}

struct file_operations vnet_fops = {
	.read = vnet_cread,
	.write = vnet_cwrite,
};

struct devclass vnet_cdev = {
	.class = "vnet",
	.type = VFS_TYPE_DEV_CHAR,
	.fops = &vnet_fops,
};

/**
 * Register the driver and its device, /dev/vnet.
 *
 * @param dev		the device
 * @param fdt_offset	its node in the device tree
 * @return 0
 */

static int vnet_init(dev_t *dev, int fdt_offset)
{
	vnet_priv_t *vnet_priv;

	devclass_register(dev, &vnet_cdev);

	vnet_priv = malloc(sizeof(vnet_priv_t));
	BUG_ON(!vnet_priv);

	memset(vnet_priv, 0, sizeof(vnet_priv_t));
	vnet_priv->vnet.ring_ref = GRANT_INVALID_REF;

	dev_set_drvdata(dev, vnet_priv);

	vdevfront_init(VNET_NAME, &vnetdrv);

	return 0;
}

REGISTER_DRIVER_POSTCORE("vnet,frontend", vnet_init);
