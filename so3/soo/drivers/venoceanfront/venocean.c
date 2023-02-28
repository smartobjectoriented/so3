/*
 * Copyright (C) 2018-2019 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2018-2019 Baptiste Delporte <bonel@bonel.net>
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

#include <soo/dev/venocean.h>

typedef struct {

	/* Must be the first field */
	venocean_t venocean;
	struct completion waitlock;

} venocean_priv_t;

static struct vbus_device *venocean_dev = NULL;

int venocean_get_data(char *buf) {
	venocean_priv_t *venocean_priv;
	venocean_response_t *ring_rsp;
	int len = 0;

	BUG_ON(!venocean_dev);

	venocean_priv = (venocean_priv_t *) dev_get_drvdata(venocean_dev->dev);

	wait_for_completion(&venocean_priv->waitlock);

	ring_rsp = venocean_get_ring_response(&venocean_priv->venocean.ring);
	BUG_ON(!ring_rsp);

	len = ring_rsp->len;
	BUG_ON(len < 1);

	BUG_ON(!buf);

	memcpy(buf, ring_rsp->buffer, len);

	return len;
}

irq_return_t venocean_interrupt(int irq, void *dev_id) {
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	venocean_priv_t *venocean_priv = dev_get_drvdata(vdev->dev);

	complete(&venocean_priv->waitlock);
	DBG(VENOCEAN_PREFIX " irq handled\n");

	return IRQ_COMPLETED;
}

void venocean_probe(struct vbus_device *vdev) {
	int res;
	unsigned int evtchn;
	venocean_sring_t *sring;
	struct vbus_transaction vbt;
	venocean_priv_t *venocean_priv;

	DBG(VENOCEAN_PREFIX " Probe\n");

	if (vdev->state == VbusStateConnected)
		return ;

	venocean_priv = dev_get_drvdata(vdev->dev);
	venocean_dev = vdev;

	DBG(VENOCEAN_PREFIX " Setup ring\n");

	init_completion(&venocean_priv->waitlock);

	/* Prepare to set up the ring. */
	venocean_priv->venocean.ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	vbus_alloc_evtchn(vdev, &evtchn);

	res = bind_evtchn_to_irq_handler(evtchn, venocean_interrupt, NULL, vdev);
	if (res <= 0) {
		lprintk("%s - line %d: Binding event channel failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	venocean_priv->venocean.evtchn = evtchn;
	venocean_priv->venocean.irq = res;

	/* Allocate a shared page for the ring */
	sring = (venocean_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&venocean_priv->venocean.ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */
	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) venocean_priv->venocean.ring.sring)));
	if (res < 0)
		BUG();

	venocean_priv->venocean.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", venocean_priv->venocean.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", venocean_priv->venocean.evtchn);

	vbus_transaction_end(vbt);

	DBG(VENOCEAN_PREFIX "  Probed successfully\n");
}

/* At this point, the FE is not connected. */
void venocean_reconfiguring(struct vbus_device *vdev) {
	int res;
	struct vbus_transaction vbt;
	venocean_priv_t *venocean_priv = dev_get_drvdata(vdev->dev);

	DBG(VENOCEAN_PREFIX  " Reconfiguring\n");
	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access_ref(venocean_priv->venocean.ring_ref);

	DBG(VENOCEAN_PREFIX " Setup ring\n");

	/* Prepare to set up the ring. */

	venocean_priv->venocean.ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(venocean_priv->venocean.ring.sring);
	FRONT_RING_INIT(&venocean_priv->venocean.ring, venocean_priv->venocean.ring.sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) venocean_priv->venocean.ring.sring)));
	BUG_ON(res < 0);

	venocean_priv->venocean.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", venocean_priv->venocean.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", venocean_priv->venocean.evtchn);

	vbus_transaction_end(vbt);
}

void venocean_shutdown(struct vbus_device *vdev) {

	DBG(VENOCEAN_PREFIX " Shutdown\n");
}

void venocean_closed(struct vbus_device *vdev) {
	venocean_priv_t *venocean_priv = dev_get_drvdata(vdev->dev);

	DBG(VENOCEAN_PREFIX " Close\n");

	/**
	 * Free the ring and deallocate the proper data.
	 */

	/* Free resources associated with old device channel. */
	if (venocean_priv->venocean.ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(venocean_priv->venocean.ring_ref);
		free_vpage((addr_t) venocean_priv->venocean.ring.sring);

		venocean_priv->venocean.ring_ref = GRANT_INVALID_REF;
		venocean_priv->venocean.ring.sring = NULL;
	}

	if (venocean_priv->venocean.irq)
		unbind_from_irqhandler(venocean_priv->venocean.irq);

	venocean_priv->venocean.irq = 0;
}

void venocean_suspend(struct vbus_device *vdev) {

	DBG(VENOCEAN_PREFIX " Suspend\n");
}

void venocean_resume(struct vbus_device *vdev) {

	DBG(VENOCEAN_PREFIX " Resume\n");
}

void venocean_connected(struct vbus_device *vdev) {
	venocean_priv_t *venocean_priv = dev_get_drvdata(vdev->dev);

	DBG(VENOCEAN_PREFIX " Connected\n");

	/* Force the processing of pending requests, if any */
	notify_remote_via_virq(venocean_priv->venocean.irq);
#if 0
		kernel_thread(notify_fn, "notify_th", NULL, 0);
#endif
}

vdrvfront_t venoceandrv = {
	.probe = venocean_probe,
	.reconfiguring = venocean_reconfiguring,
	.shutdown = venocean_shutdown,
	.closed = venocean_closed,
	.suspend = venocean_suspend,
	.resume = venocean_resume,
	.connected = venocean_connected
};

static int venocean_init(dev_t *dev, int fdt_offset) {
	venocean_priv_t *venocean_priv;

	venocean_priv = malloc(sizeof(venocean_priv_t));
	BUG_ON(!venocean_priv);

	memset(venocean_priv, 0, sizeof(venocean_priv_t));

	dev_set_drvdata(dev, venocean_priv);

	vdevfront_init(VENOCEAN_NAME, &venoceandrv);
	
	DBG(VENOCEAN_PREFIX " Initialized successfully\n");

	return 0;
}
REGISTER_DRIVER_POSTCORE("venocean,frontend", venocean_init);
