/*
 * Copyright (C) 2022 Mattia Gallacchi <mattia.gallaccchi@heig-vd.ch>
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

#if 0
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

#include <soo/dev/vwagoled.h>

typedef struct {

	/* Must be the first field */
	vwagoled_t vwagoled;

} vwagoled_priv_t;

static struct vbus_device *vwagoled_dev = NULL;

/**
 * @brief Set default values for a request struct
 * 
 * @param req request to initialize
 */
void init_request(vwagoled_request_t *req) {
	req->cmd = CMD_NONE;
	req->dim_value = DEFAULT_DIM_VALUE;
	memset(req->ids, -1, sizeof(int) * VWAGOLED_PACKET_SIZE);
}

int vwagoled_set(int *ids, int size, wago_cmd_t cmd) {
	vwagoled_priv_t *vwagoled_priv;
	vwagoled_request_t *ring_req;
	
	BUG_ON(!vwagoled_dev);

	BUG_ON(size > VWAGOLED_PACKET_SIZE);

	vwagoled_priv = (vwagoled_priv_t *) dev_get_drvdata(vwagoled_dev->dev);

	vdevfront_processing_begin(vwagoled_dev);

	if (!RING_REQ_FULL(&vwagoled_priv->vwagoled.ring)) {
		ring_req = vwagoled_new_ring_request(&vwagoled_priv->vwagoled.ring);

		init_request(ring_req);

		ring_req->cmd = (int)cmd;
		ring_req->ids_count = size;
		memcpy(ring_req->ids, ids, size * sizeof(int));

		vwagoled_ring_request_ready(&vwagoled_priv->vwagoled.ring);

		notify_remote_via_virq(vwagoled_priv->vwagoled.irq);

	} else {
		DBG("Ring full\n");
		BUG();
	}

	vdevfront_processing_end(vwagoled_dev);

	return 0;
}

irq_return_t vwagoled_interrupt(int irq, void *dev_id) {
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	vwagoled_priv_t *vwagoled_priv = dev_get_drvdata(vdev->dev);
	vwagoled_response_t *ring_rsp;

	DBG("%s, %d\n", __func__, ME_domID());

	while ((ring_rsp = vwagoled_get_ring_response(&vwagoled_priv->vwagoled.ring)) != NULL) {

		DBG("%s, cons=%d\n", __func__);

		/* Do something with the response */

#if 0 /* Debug */
		lprintk("## Got from the backend: %s\n", ring_rsp->buffer);
#endif
	}

	return IRQ_COMPLETED;
}


static void vwagoled_probe(struct vbus_device *vdev) {
	int res;
	unsigned int evtchn;
	vwagoled_sring_t *sring;
	struct vbus_transaction vbt;
	vwagoled_priv_t *vwagoled_priv;

	DBG0(VWAGOLED_PREFIX " Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return ;

	vwagoled_priv = dev_get_drvdata(vdev->dev);
	vwagoled_dev = vdev;

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vwagoled_priv->vwagoled.ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	vbus_alloc_evtchn(vdev, &evtchn);

	res = bind_evtchn_to_irq_handler(evtchn, vwagoled_interrupt, NULL, vdev);
	if (res <= 0) {
		lprintk("%s - line %d: Binding event channel failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	vwagoled_priv->vwagoled.evtchn = evtchn;
	vwagoled_priv->vwagoled.irq = res;

	/* Allocate a shared page for the ring */
	sring = (vwagoled_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&vwagoled_priv->vwagoled.ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vwagoled_priv->vwagoled.ring.sring)));
	if (res < 0)
		BUG();

	vwagoled_priv->vwagoled.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vwagoled_priv->vwagoled.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vwagoled_priv->vwagoled.evtchn);

	vbus_transaction_end(vbt);

	DBG(VWAGOLED_PREFIX " Frontend probed successfully\n");
}

/* At this point, the FE is not connected. */
static void vwagoled_reconfiguring(struct vbus_device *vdev) {
	int res;
	struct vbus_transaction vbt;
	vwagoled_priv_t *vwagoled_priv = dev_get_drvdata(vdev->dev);

	DBG0(VWAGOLED_PREFIX " Frontend reconfiguring\n");
	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access_ref(vwagoled_priv->vwagoled.ring_ref);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vwagoled_priv->vwagoled.ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(vwagoled_priv->vwagoled.ring.sring);
	FRONT_RING_INIT(&vwagoled_priv->vwagoled.ring, vwagoled_priv->vwagoled.ring.sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vwagoled_priv->vwagoled.ring.sring)));
	if (res < 0)
		BUG();

	vwagoled_priv->vwagoled.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vwagoled_priv->vwagoled.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vwagoled_priv->vwagoled.evtchn);

	vbus_transaction_end(vbt);
}

static void vwagoled_shutdown(struct vbus_device *vdev) {

	DBG0(VWAGOLED_PREFIX " Frontend shutdown\n");
}

static void vwagoled_closed(struct vbus_device *vdev) {
	vwagoled_priv_t *vwagoled_priv = dev_get_drvdata(vdev->dev);

	DBG0(VWAGOLED_PREFIX " Frontend close\n");

	/**
	 * Free the ring and deallocate the proper data.
	 */

	/* Free resources associated with old device channel. */
	if (vwagoled_priv->vwagoled.ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(vwagoled_priv->vwagoled.ring_ref);
		free_vpage((addr_t) vwagoled_priv->vwagoled.ring.sring);

		vwagoled_priv->vwagoled.ring_ref = GRANT_INVALID_REF;
		vwagoled_priv->vwagoled.ring.sring = NULL;
	}

	if (vwagoled_priv->vwagoled.irq)
		unbind_from_irqhandler(vwagoled_priv->vwagoled.irq);

	vwagoled_priv->vwagoled.irq = 0;
}

static void vwagoled_suspend(struct vbus_device *vdev) {

	DBG0(VWAGOLED_PREFIX " Frontend suspend\n");
}

static void vwagoled_resume(struct vbus_device *vdev) {

	DBG0(VWAGOLED_PREFIX " Frontend resume\n");
}

static void vwagoled_connected(struct vbus_device *vdev) {
	vwagoled_priv_t *vwagoled_priv = dev_get_drvdata(vdev->dev);

	DBG0(VWAGOLED_PREFIX " Frontend connected\n");

	/* Force the processing of pending requests, if any */
	notify_remote_via_virq(vwagoled_priv->vwagoled.irq);
}

vdrvfront_t vwagoleddrv = {
	.probe = vwagoled_probe,
	.reconfiguring = vwagoled_reconfiguring,
	.shutdown = vwagoled_shutdown,
	.closed = vwagoled_closed,
	.suspend = vwagoled_suspend,
	.resume = vwagoled_resume,
	.connected = vwagoled_connected
};

static int vwagoled_init(dev_t *dev, int fdt_offset) {
	vwagoled_priv_t *vwagoled_priv;

	vwagoled_priv = malloc(sizeof(vwagoled_priv_t));
	BUG_ON(!vwagoled_priv);

	memset(vwagoled_priv, 0, sizeof(vwagoled_priv_t));

	dev_set_drvdata(dev, vwagoled_priv);

	vdevfront_init(VWAGOLED_NAME, &vwagoleddrv);

	DBG(VWAGOLED_PREFIX " initialized successfully\n");

	return 0;
}

REGISTER_DRIVER_POSTCORE("vwagoled,frontend", vwagoled_init);
