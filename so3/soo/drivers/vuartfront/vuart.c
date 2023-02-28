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

#include <soo/dev/vuart.h>

typedef struct {

	/* Must be the first field */
	vuart_t vuart;

	completion_t reader_wait;

} vuart_priv_t;

/* Our unique uart instance. */
static struct vbus_device *vdev_console = NULL;

irq_return_t vuart_interrupt(int irq, void *dev_id) {
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	vuart_priv_t *vuart_priv = (vuart_priv_t *) dev_get_drvdata(vdev->dev);

	complete(&vuart_priv->reader_wait);

	return IRQ_COMPLETED;
}

/*
 * Can be used outside the frontend by other subsystems.
 */
bool vuart_ready(void) {
	return (vdev_console && (vdev_console->state == VbusStateConnected));
}

/**
 * Send a string on the vuart device.
 */
void vuart_write(char *buffer, int count) {
	int i;
	vuart_request_t *ring_req;
	vuart_priv_t *vuart_priv;

	if (!vdev_console)
		return ;

	vuart_priv = (vuart_priv_t *) dev_get_drvdata(vdev_console->dev);
	BUG_ON(!vuart_priv);

	vdevfront_processing_begin(vdev_console);

	for (i = 0; i < count; i++) {
		ring_req = vuart_new_ring_request(&vuart_priv->vuart.ring);

		ring_req->c = buffer[i];
	}

	vuart_ring_request_ready(&vuart_priv->vuart.ring);

	notify_remote_via_virq(vuart_priv->vuart.irq);

	vdevfront_processing_end(vdev_console);

}

/*
 * The only way to get a char from the backend is
 * along the vuart interrupt path. Hence, an interrupt must be raised up
 * in any case.
 */
char vuart_read_char(void) {
	vuart_response_t *ring_rsp;
	vuart_priv_t *vuart_priv;

	if (!vdev_console)
		return 0;

	vuart_priv = (vuart_priv_t *) dev_get_drvdata(vdev_console->dev);
	BUG_ON(!vuart_priv);

	/* Always perform a wait on the completion since we always get an interrupt
	 * per byte (hence a complete will be aised up).
	 */
	wait_for_completion(&vuart_priv->reader_wait);

	vdevfront_processing_begin(vdev_console);

	ring_rsp = vuart_get_ring_response(&vuart_priv->vuart.ring);
	BUG_ON(!ring_rsp);

	vdevfront_processing_end(vdev_console);

	return ring_rsp->c;
}

void vuart_probe(struct vbus_device *vdev) {
	unsigned int evtchn;
	vuart_sring_t *sring;
	struct vbus_transaction vbt;
	vuart_priv_t *vuart_priv;

	DBG0("[vuart] Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return ;

	vuart_priv = dev_get_drvdata(vdev->dev);

	/* Local instance */
	vdev_console = vdev;

	init_completion(&vuart_priv->reader_wait);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vuart_priv->vuart.ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	vbus_alloc_evtchn(vdev, &evtchn);

	vuart_priv->vuart.irq = bind_evtchn_to_irq_handler(evtchn, vuart_interrupt, NULL, vdev);
	vuart_priv->vuart.evtchn = evtchn;

	/* Allocate a shared page for the ring */
	sring = (vuart_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&vuart_priv->vuart.ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	vuart_priv->vuart.ring_ref = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vuart_priv->vuart.ring.sring)));

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vuart_priv->vuart.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vuart_priv->vuart.evtchn);

	vbus_transaction_end(vbt);

}

/* At this point, the FE is not connected. */
void vuart_reconfiguring(struct vbus_device *vdev) {
	int res;
	struct vbus_transaction vbt;
	vuart_priv_t *vuart_priv = dev_get_drvdata(vdev->dev);

	DBG0("[vuart] Frontend reconfiguring\n");
	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access_ref(vuart_priv->vuart.ring_ref);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vuart_priv->vuart.ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(vuart_priv->vuart.ring.sring);
	FRONT_RING_INIT(&vuart_priv->vuart.ring, vuart_priv->vuart.ring.sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vuart_priv->vuart.ring.sring)));
	if (res < 0)
		BUG();

	vuart_priv->vuart.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vuart_priv->vuart.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vuart_priv->vuart.evtchn);

	vbus_transaction_end(vbt);
}

void vuart_shutdown(struct vbus_device *vdev) {

	DBG0("[vuart] Frontend shutdown\n");
}

void vuart_closed(struct vbus_device *vdev) {
	vuart_priv_t *vuart_priv = dev_get_drvdata(vdev->dev);

	DBG0("[vuart] Frontend close\n");

	/**
	 * Free the ring and deallocate the proper data.
	 */

	/* Free resources associated with old device channel. */
	if (vuart_priv->vuart.ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(vuart_priv->vuart.ring_ref);
		free_vpage((addr_t) vuart_priv->vuart.ring.sring);

		vuart_priv->vuart.ring_ref = GRANT_INVALID_REF;
		vuart_priv->vuart.ring.sring = NULL;
	}

	if (vuart_priv->vuart.irq)
		unbind_from_irqhandler(vuart_priv->vuart.irq);

	vuart_priv->vuart.irq = 0;
}

void vuart_suspend(struct vbus_device *vdev) {

	DBG0("[vuart] Frontend suspend\n");
}

void vuart_resume(struct vbus_device *vdev) {

	DBG0("[vuart] Frontend resume\n");
}

void vuart_connected(struct vbus_device *vdev) {

	DBG0("[vuart] Frontend connected\n");

}

vdrvfront_t vuartdrv = {
	.probe = vuart_probe,
	.reconfiguring = vuart_reconfiguring,
	.shutdown = vuart_shutdown,
	.closed = vuart_closed,
	.suspend = vuart_suspend,
	.resume = vuart_resume,
	.connected = vuart_connected
};

static int vuart_init(dev_t *dev, int fdt_offset) {
	vuart_priv_t *vuart_priv;

	vuart_priv = malloc(sizeof(vuart_priv_t));
	BUG_ON(!vuart_priv);

	memset(vuart_priv, 0, sizeof(vuart_priv_t));

	dev_set_drvdata(dev, vuart_priv);

	vdevfront_init(VUART_NAME, &vuartdrv);

	return 0;
}

REGISTER_DRIVER_POSTCORE("vuart,frontend", vuart_init);
