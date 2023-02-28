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

#include <soo/dev/vsensej.h>

typedef struct {

	/* Must be the first field */
	vsensej_t vsensej;
	struct completion waitlock;

} vsensej_priv_t;

static struct vbus_device *vsensej_dev = NULL;

/**
 * Get a key from the Sense HAT joystick.
 *
 * @param ie will contain the resulting key according to the struct input_event Linux type.
 * @return 0 if successful, -1 if not ready yet.
 */
int vsensej_get(struct input_event *ie) {
	vsensej_priv_t *vsensej_priv;
	vsensej_response_t *ring_rsp;

	if (!vsensej_dev)
		return -1;

	vsensej_priv = (vsensej_priv_t *) dev_get_drvdata(vsensej_dev->dev);

	wait_for_completion(&vsensej_priv->waitlock);

	ring_rsp = vsensej_get_ring_response(&vsensej_priv->vsensej.ring);

	ie->type = ring_rsp->type;
	ie->code = ring_rsp->code;
	ie->value = ring_rsp->value;

	return sizeof(struct input_event);
}

irq_return_t vsensej_interrupt(int irq, void *dev_id) {
	vsensej_priv_t *vsensej_priv = (vsensej_priv_t *) dev_get_drvdata(vsensej_dev->dev);

	complete(&vsensej_priv->waitlock);

	return IRQ_COMPLETED;
}

static void vsensej_probe(struct vbus_device *vdev) {
	unsigned int evtchn;
	vsensej_sring_t *sring;
	struct vbus_transaction vbt;
	vsensej_priv_t *vsensej_priv;

	DBG0("[" VSENSEJ_NAME "] Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return ;

	vsensej_priv = dev_get_drvdata(vdev->dev);

	vsensej_dev = vdev;

	DBG("Frontend: Setup ring\n");

	init_completion(&vsensej_priv->waitlock);

	/* Prepare to set up the ring. */

	vsensej_priv->vsensej.ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	vbus_alloc_evtchn(vdev, &evtchn);

	vsensej_priv->vsensej.irq = bind_evtchn_to_irq_handler(evtchn, vsensej_interrupt, NULL, vdev);
	vsensej_priv->vsensej.evtchn = evtchn;

	/* Allocate a shared page for the ring */
	sring = (vsensej_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&vsensej_priv->vsensej.ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	vsensej_priv->vsensej.ring_ref = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vsensej_priv->vsensej.ring.sring)));

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vsensej_priv->vsensej.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vsensej_priv->vsensej.evtchn);

	vbus_transaction_end(vbt);

}

/* At this point, the FE is not connected. */
static void vsensej_reconfiguring(struct vbus_device *vdev) {
	int res;
	struct vbus_transaction vbt;
	vsensej_priv_t *vsensej_priv = dev_get_drvdata(vdev->dev);

	DBG0("[" VSENSEJ_NAME "] Frontend reconfiguring\n");
	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access_ref(vsensej_priv->vsensej.ring_ref);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vsensej_priv->vsensej.ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(vsensej_priv->vsensej.ring.sring);
	FRONT_RING_INIT(&vsensej_priv->vsensej.ring, vsensej_priv->vsensej.ring.sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vsensej_priv->vsensej.ring.sring)));
	if (res < 0)
		BUG();

	vsensej_priv->vsensej.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vsensej_priv->vsensej.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vsensej_priv->vsensej.evtchn);

	vbus_transaction_end(vbt);
}

static void vsensej_shutdown(struct vbus_device *vdev) {

	DBG0("[" VSENSEJ_NAME "] Frontend shutdown\n");
}

static void vsensej_closed(struct vbus_device *vdev) {
	vsensej_priv_t *vsensej_priv = dev_get_drvdata(vdev->dev);

	DBG0("[" VSENSEJ_NAME "] Frontend close\n");

	/**
	 * Free the ring and deallocate the proper data.
	 */

	/* Free resources associated with old device channel. */
	if (vsensej_priv->vsensej.ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(vsensej_priv->vsensej.ring_ref);
		free_vpage((addr_t) vsensej_priv->vsensej.ring.sring);

		vsensej_priv->vsensej.ring_ref = GRANT_INVALID_REF;
		vsensej_priv->vsensej.ring.sring = NULL;
	}

	if (vsensej_priv->vsensej.irq)
		unbind_from_irqhandler(vsensej_priv->vsensej.irq);

	vsensej_priv->vsensej.irq = 0;
}

static void vsensej_suspend(struct vbus_device *vdev) {

	DBG0("[" VSENSEJ_NAME "] Frontend suspend\n");
}

static void vsensej_resume(struct vbus_device *vdev) {

	DBG0("[" VSENSEJ_NAME "] Frontend resume\n");
}

static void vsensej_connected(struct vbus_device *vdev) {

	DBG0("[" VSENSEJ_NAME "] Frontend connected\n");

}

vdrvfront_t vsensejdrv = {
	.probe = vsensej_probe,
	.reconfiguring = vsensej_reconfiguring,
	.shutdown = vsensej_shutdown,
	.closed = vsensej_closed,
	.suspend = vsensej_suspend,
	.resume = vsensej_resume,
	.connected = vsensej_connected
};

static int vsensej_init(dev_t *dev, int fdt_offset) {
	vsensej_priv_t *vsensej_priv;

	vsensej_priv = malloc(sizeof(vsensej_priv_t));
	BUG_ON(!vsensej_priv);

	memset(vsensej_priv, 0, sizeof(vsensej_priv_t));

	dev_set_drvdata(dev, vsensej_priv);

	vdevfront_init(VSENSEJ_NAME, &vsensejdrv);

	return 0;
}

REGISTER_DRIVER_POSTCORE("vsensej,frontend", vsensej_init);
