/*
 * Copyright (C) 2023 A.Gabriel Catel Torres <arzur.cateltorres@heig-vd.ch>
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
 * Description: This file is the implementation of the front end for the IUOC 
 * application. There is the definition of the structure used to transfer
 * and decode the data. There are also functions to send and retrieve any
 * data that will be used to control smart objects. 
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

#include <soo/dev/viuoc.h>

typedef struct {

	/* Must be the first field */
	viuoc_t viuoc;

	struct completion waitlock;
} viuoc_priv_t;

static struct vbus_device *viuoc_dev = NULL;

int viuoc_set(iuoc_data_t me_data) {
	
	viuoc_priv_t *viuoc_priv;
	viuoc_request_t *ring_req;
	
	BUG_ON(!viuoc_dev);

	viuoc_priv = (viuoc_priv_t *) dev_get_drvdata(viuoc_dev->dev);

	vdevfront_processing_begin(viuoc_dev);

	if (!RING_REQ_FULL(&viuoc_priv->viuoc.ring)) {
		ring_req = viuoc_new_ring_request(&viuoc_priv->viuoc.ring);

		ring_req->me_data = me_data;

		viuoc_ring_request_ready(&viuoc_priv->viuoc.ring);

		notify_remote_via_virq(viuoc_priv->viuoc.irq);

	} else {
		DBG("Ring full\n");
		BUG();
	}

	vdevfront_processing_end(viuoc_dev);

	return 0;
}

int get_iuoc_me_data(iuoc_data_t *data) {
	viuoc_priv_t *viuoc_priv = dev_get_drvdata(viuoc_dev->dev);
	viuoc_response_t *ring_rsp;
	int ret = 0;

	DBG("[IUOC front]: Waiting for a new data\n");
	wait_for_completion(&viuoc_priv->waitlock);

	DBG("[IUOC front]: New data received\n");
	vdevfront_processing_begin(viuoc_dev);

	if ((ring_rsp = viuoc_get_ring_response(&viuoc_priv->viuoc.ring)) != NULL) {
		memcpy(data, &(ring_rsp->me_data), sizeof(iuoc_data_t));
	} else {
		ret = -1;
	}

	vdevfront_processing_end(viuoc_dev);

	return ret;
}

irq_return_t viuoc_interrupt(int irq, void *dev_id) {
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	viuoc_priv_t *viuoc_priv = dev_get_drvdata(vdev->dev);

	//DBG(VIUOC_PREFIX "%s, %d\n", __func__, ME_domID());
	complete(&viuoc_priv->waitlock);
	DBG("[IUOC front]: INTERRUPT TRIGGERED\n");

	return IRQ_COMPLETED;
}


static void viuoc_probe(struct vbus_device *vdev) {
	int res;
	unsigned int evtchn;
	viuoc_sring_t *sring;
	struct vbus_transaction vbt;
	viuoc_priv_t *viuoc_priv;

	DBG(VIUOC_PREFIX " Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return ;

	viuoc_priv = dev_get_drvdata(vdev->dev);
	viuoc_dev = vdev;

	init_completion(&viuoc_priv->waitlock);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	viuoc_priv->viuoc.ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	vbus_alloc_evtchn(vdev, &evtchn);

	res = bind_evtchn_to_irq_handler(evtchn, viuoc_interrupt, NULL, vdev);
	if (res <= 0) {
		lprintk("%s - line %d: Binding event channel failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	viuoc_priv->viuoc.evtchn = evtchn;
	viuoc_priv->viuoc.irq = res;

	/* Allocate a shared page for the ring */
	sring = (viuoc_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&viuoc_priv->viuoc.ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) viuoc_priv->viuoc.ring.sring)));
	if (res < 0)
		BUG();

	viuoc_priv->viuoc.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", viuoc_priv->viuoc.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", viuoc_priv->viuoc.evtchn);

	vbus_transaction_end(vbt);

	DBG(VIUOC_PREFIX " Frontend probed successfully\n");
}

/* At this point, the FE is not connected. */
static void viuoc_reconfiguring(struct vbus_device *vdev) {
	int res;
	struct vbus_transaction vbt;
	viuoc_priv_t *viuoc_priv = dev_get_drvdata(vdev->dev);

	DBG0(VIUOC_PREFIX " Frontend reconfiguring\n");
	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access_ref(viuoc_priv->viuoc.ring_ref);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	viuoc_priv->viuoc.ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(viuoc_priv->viuoc.ring.sring);
	FRONT_RING_INIT(&viuoc_priv->viuoc.ring, viuoc_priv->viuoc.ring.sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) viuoc_priv->viuoc.ring.sring)));
	if (res < 0)
		BUG();

	viuoc_priv->viuoc.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", viuoc_priv->viuoc.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", viuoc_priv->viuoc.evtchn);

	vbus_transaction_end(vbt);
}

static void viuoc_shutdown(struct vbus_device *vdev) {

	DBG0(VIUOC_PREFIX " Frontend shutdown\n");
}

static void viuoc_closed(struct vbus_device *vdev) {
	viuoc_priv_t *viuoc_priv = dev_get_drvdata(vdev->dev);

	DBG0(VIUOC_PREFIX " Frontend close\n");

	/**
	 * Free the ring and deallocate the proper data.
	 */

	/* Free resources associated with old device channel. */
	if (viuoc_priv->viuoc.ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(viuoc_priv->viuoc.ring_ref);
		free_vpage((addr_t) viuoc_priv->viuoc.ring.sring);

		viuoc_priv->viuoc.ring_ref = GRANT_INVALID_REF;
		viuoc_priv->viuoc.ring.sring = NULL;
	}

	if (viuoc_priv->viuoc.irq)
		unbind_from_irqhandler(viuoc_priv->viuoc.irq);

	viuoc_priv->viuoc.irq = 0;
}

static void viuoc_suspend(struct vbus_device *vdev) {

	DBG0(VIUOC_PREFIX " Frontend suspend\n");
}

static void viuoc_resume(struct vbus_device *vdev) {

	DBG0(VIUOC_PREFIX " Frontend resume\n");
}

static void viuoc_connected(struct vbus_device *vdev) {
	viuoc_priv_t *viuoc_priv = dev_get_drvdata(vdev->dev);

	DBG0(VIUOC_PREFIX " Frontend connected\n");

	/* Force the processing of pending requests, if any */
	notify_remote_via_virq(viuoc_priv->viuoc.irq);
}

vdrvfront_t viuocdrv = {
	.probe = viuoc_probe,
	.reconfiguring = viuoc_reconfiguring,
	.shutdown = viuoc_shutdown,
	.closed = viuoc_closed,
	.suspend = viuoc_suspend,
	.resume = viuoc_resume,
	.connected = viuoc_connected
};

static int viuoc_init(dev_t *dev, int fdt_offset) {
	viuoc_priv_t *viuoc_priv;
	viuoc_priv = malloc(sizeof(viuoc_priv_t));
	BUG_ON(!viuoc_priv);

	memset(viuoc_priv, 0, sizeof(viuoc_priv_t));

	dev_set_drvdata(dev, viuoc_priv);

	vdevfront_init(VIUOC_NAME, &viuocdrv);

	DBG(VIUOC_PREFIX " initialized successfully\n");

	return 0;
}

REGISTER_DRIVER_POSTCORE("viuoc,frontend", viuoc_init);
