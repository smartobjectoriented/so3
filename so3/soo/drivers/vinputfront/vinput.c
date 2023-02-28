/*
 * Copyright (C) 2020 Nikolaos Garanis <nikolaos.garanis@heig-vd.ch>
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
#include <device/input/so3virt_mse.h>
#include <device/input/so3virt_kbd.h>

#include <soo/evtchn.h>
#include <soo/gnttab.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/console.h>
#include <soo/debug.h>
#include <soo/dev/vinput.h>
#include <uapi/linux/input-event-codes.h>

irq_return_t vinput_interrupt(int irq, void *dev_id)
{
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	vinput_t *vinput = to_vinput(vdev);
	vinput_response_t *ring_rsp;
	unsigned int type, code;
	int value;

	while ((ring_rsp = vinput_ring_response(&vinput->ring)) != NULL) {

		type = ring_rsp->type;
		code = ring_rsp->code;
		value = ring_rsp->value;

		if (type == EV_REL || /* is mouse movement */
		   (type == EV_KEY && /* is mouse button click */
			(code == BTN_LEFT || code == BTN_MIDDLE || code == BTN_RIGHT || code == BTN_TOUCH)) ||
		   (type == EV_ABS && /* is touchscreen event */
			(code == ABS_X || code == ABS_Y))) {

			so3virt_mse_event(type, code, value);
		}
		else if (type == EV_KEY) {
			so3virt_kbd_event(type, code, value);
		}

		/* Skip unhandled events. */
	}

	return IRQ_COMPLETED;
}

void vinput_probe(struct vbus_device *vdev)
{
	int res;
	unsigned int evtchn;
	vinput_sring_t *sring;
	struct vbus_transaction vbt;
	vinput_t *vinput;

	DBG0("[" VINPUT_NAME "] Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return ;

	vinput = malloc(sizeof(vinput_t));
	BUG_ON(!vinput);
	memset(vinput, 0, sizeof(vinput_t));

	dev_set_drvdata(vdev->dev, &vinput->vdevfront);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vinput->ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	res = vbus_alloc_evtchn(vdev, &evtchn);
	BUG_ON(res);

	res = bind_evtchn_to_irq_handler(evtchn, vinput_interrupt, NULL, vdev);
	if (res <= 0) {
		lprintk("%s - line %d: Binding event channel failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	vinput->evtchn = evtchn;
	vinput->irq = res;

	/* Allocate a shared page for the ring */
	sring = (vinput_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&vinput->ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((uint32_t) vinput->ring.sring)));
	if (res < 0)
		BUG();

	vinput->ring_ref = res;

	vbus_transaction_start(&vbt);
	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vinput->ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vinput->evtchn);
	vbus_transaction_end(vbt);
}

/* At this point, the FE is not connected. */
void vinput_reconfiguring(struct vbus_device *vdev)
{
	int res;
	struct vbus_transaction vbt;
	vinput_t *vinput = to_vinput(vdev);

	DBG0("[" VINPUT_NAME "] Frontend reconfiguring\n");
	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access_ref(vinput->ring_ref);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vinput->ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(vinput->ring.sring);
	FRONT_RING_INIT(&vinput->ring, (&vinput->ring)->sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((uint32_t) vinput->ring.sring)));
	BUG_ON(res < 0);

	vinput->ring_ref = res;

	vbus_transaction_start(&vbt);
	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vinput->ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vinput->evtchn);
	vbus_transaction_end(vbt);
}

void vinput_shutdown(struct vbus_device *vdev)
{
	DBG0("[" VINPUT_NAME "] Frontend shutdown\n");
}

void vinput_closed(struct vbus_device *vdev)
{
	vinput_t *vinput = to_vinput(vdev);

	DBG0("[" VINPUT_NAME "] Frontend close\n");

	/* Free the ring and deallocate the proper data. */

	/* Free resources associated with old device channel. */
	if (vinput->ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(vinput->ring_ref);
		free_vpage((uint32_t) vinput->ring.sring);

		vinput->ring_ref = GRANT_INVALID_REF;
		vinput->ring.sring = NULL;
	}

	if (vinput->irq)
		unbind_from_irqhandler(vinput->irq);

	vinput->irq = 0;
}

void vinput_suspend(struct vbus_device *vdev)
{
	DBG0("[" VINPUT_NAME "] Frontend suspend\n");
}

void vinput_resume(struct vbus_device *vdev)
{
	DBG0("[" VINPUT_NAME "] Frontend resume\n");
}

void vinput_connected(struct vbus_device *vdev)
{
	vinput_t *vinput = to_vinput(vdev);

	DBG0("[" VINPUT_NAME "] Frontend connected\n");

	/* Force the processing of pending requests, if any */
	notify_remote_via_virq(vinput->irq);
}

vdrvfront_t vinputdrv = {
	.probe = vinput_probe,
	.reconfiguring = vinput_reconfiguring,
	.shutdown = vinput_shutdown,
	.closed = vinput_closed,
	.suspend = vinput_suspend,
	.resume = vinput_resume,
	.connected = vinput_connected
};

static int vinput_init(dev_t *dev) {
	vdevfront_init(VINPUT_NAME, &vinputdrv);
	return 0;
}

REGISTER_DRIVER_POSTCORE("vinput,frontend", vinput_init);
