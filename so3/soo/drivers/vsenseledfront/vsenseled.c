/*
 * Copyright (C) 2021 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <soo/dev/vsenseled.h>

typedef struct {

	/* Must be the first field */
	vsenseled_t vsenseled;

} vsenseled_priv_t;

static struct vbus_device *vsenseled_dev = NULL;

/**
 * Switch on/off a (logical) led on the Sense HAT led matrix.
 * By logical led, we hear a square of 4 leds to match with
 * the joystick position.
 *
 * @param lednr		between 0 and 4
 * @param ledstate	true = on, false = off
 */
void vsenseled_set(int lednr, bool ledstate) {
	vsenseled_request_t *ring_req;
	vsenseled_priv_t *vsenseled_priv;

	if (!vsenseled_dev)
		return ;

	vsenseled_priv = (vsenseled_priv_t *) dev_get_drvdata(vsenseled_dev->dev);

	vdevfront_processing_begin(vsenseled_dev);

	ring_req = vsenseled_new_ring_request(&vsenseled_priv->vsenseled.ring);

	ring_req->lednr = lednr;
	ring_req->ledstate = ledstate;

	vsenseled_ring_request_ready(&vsenseled_priv->vsenseled.ring);

	notify_remote_via_virq(vsenseled_priv->vsenseled.irq);

	vdevfront_processing_end(vsenseled_dev);

}

static void vsenseled_probe(struct vbus_device *vdev) {
	unsigned int evtchn;
	vsenseled_sring_t *sring;
	struct vbus_transaction vbt;
	vsenseled_priv_t *vsenseled_priv;

	DBG0("[" VSENSELED_NAME "] Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return ;

	vsenseled_priv = dev_get_drvdata(vdev->dev);

	vsenseled_dev = vdev;

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vsenseled_priv->vsenseled.ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	vbus_alloc_evtchn(vdev, &evtchn);

	vsenseled_priv->vsenseled.irq = bind_evtchn_to_irq_handler(evtchn, NULL, NULL, vdev);
	vsenseled_priv->vsenseled.evtchn = evtchn;

	/* Allocate a shared page for the ring */
	sring = (vsenseled_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&vsenseled_priv->vsenseled.ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	vsenseled_priv->vsenseled.ring_ref = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vsenseled_priv->vsenseled.ring.sring)));

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vsenseled_priv->vsenseled.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vsenseled_priv->vsenseled.evtchn);

	vbus_transaction_end(vbt);

}

/* At this point, the FE is not connected. */
static void vsenseled_reconfiguring(struct vbus_device *vdev) {
	int res;
	struct vbus_transaction vbt;
	vsenseled_priv_t *vsenseled_priv = dev_get_drvdata(vdev->dev);

	DBG0("[" VSENSELED_NAME "] Frontend reconfiguring\n");
	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access_ref(vsenseled_priv->vsenseled.ring_ref);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vsenseled_priv->vsenseled.ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(vsenseled_priv->vsenseled.ring.sring);
	FRONT_RING_INIT(&vsenseled_priv->vsenseled.ring, vsenseled_priv->vsenseled.ring.sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vsenseled_priv->vsenseled.ring.sring)));
	if (res < 0)
		BUG();

	vsenseled_priv->vsenseled.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vsenseled_priv->vsenseled.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vsenseled_priv->vsenseled.evtchn);

	vbus_transaction_end(vbt);
}

static void vsenseled_shutdown(struct vbus_device *vdev) {

	DBG0("[" VSENSELED_NAME "] Frontend shutdown\n");
}

static void vsenseled_closed(struct vbus_device *vdev) {
	vsenseled_priv_t *vsenseled_priv = dev_get_drvdata(vdev->dev);

	DBG0("[" VSENSELED_NAME "] Frontend close\n");

	/**
	 * Free the ring and deallocate the proper data.
	 */

	/* Free resources associated with old device channel. */
	if (vsenseled_priv->vsenseled.ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(vsenseled_priv->vsenseled.ring_ref);
		free_vpage((addr_t) vsenseled_priv->vsenseled.ring.sring);

		vsenseled_priv->vsenseled.ring_ref = GRANT_INVALID_REF;
		vsenseled_priv->vsenseled.ring.sring = NULL;
	}

	if (vsenseled_priv->vsenseled.irq)
		unbind_from_irqhandler(vsenseled_priv->vsenseled.irq);

	vsenseled_priv->vsenseled.irq = 0;
}

static void vsenseled_suspend(struct vbus_device *vdev) {

	DBG0("[" VSENSELED_NAME "] Frontend suspend\n");
}

static void vsenseled_resume(struct vbus_device *vdev) {

	DBG0("[" VSENSELED_NAME "] Frontend resume\n");
}

static void vsenseled_connected(struct vbus_device *vdev) {

	DBG0("[" VSENSELED_NAME "] Frontend connected\n");

}

vdrvfront_t vsenseleddrv = {
	.probe = vsenseled_probe,
	.reconfiguring = vsenseled_reconfiguring,
	.shutdown = vsenseled_shutdown,
	.closed = vsenseled_closed,
	.suspend = vsenseled_suspend,
	.resume = vsenseled_resume,
	.connected = vsenseled_connected
};

static int vsenseled_init(dev_t *dev, int fdt_offset) {
	vsenseled_priv_t *vsenseled_priv;

	vsenseled_priv = malloc(sizeof(vsenseled_priv_t));
	BUG_ON(!vsenseled_priv);

	memset(vsenseled_priv, 0, sizeof(vsenseled_priv_t));

	dev_set_drvdata(dev, vsenseled_priv);

	vdevfront_init(VSENSELED_NAME, &vsenseleddrv);

	return 0;
}

REGISTER_DRIVER_POSTCORE("vsenseled,frontend", vsenseled_init);
