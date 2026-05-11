/*
 * Copyright (C) 2026 Clément Dieperink <clement.dieperink@heig-vd.ch>
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

#if 1
#define DEBUG
#endif

#include <heap.h>
#include <mutex.h>
#include <delay.h>
#include <memory.h>
#include <atomic.h>

#include <asm/mmu.h>

#include <device/driver.h>

#include <soo/evtchn.h>
#include <soo/gnttab.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/debug.h>

#include <soo/dev/vinput.h>

#include <uapi/linux/input-event-codes.h>

typedef struct {
	/* Must be the first field */
	vinput_t vinput;

	tcb_t *reader_thread;
	completion_t reader_wait;
	atomic_t running;

} vinput_priv_t;

/* Our unique instance. */
static struct vbus_device *vdev_console = NULL;

static irq_return_t vinput_interrupt(int irq, void *dev_id)
{
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	vinput_priv_t *vinput_priv = (vinput_priv_t *) dev_get_drvdata(vdev->dev);

	complete(&vinput_priv->reader_wait);

	return IRQ_COMPLETED;
}

static int get_source(vinput_response_t *ring_rsp)
{
	switch (ring_rsp->type) {
	case EV_ABS:
	case EV_REL:
		return SRC_MOUSE;

	case EV_KEY:
		/* Mouse button codes are before joystick codes */
		if (BTN_MOUSE <= ring_rsp->code && ring_rsp->code < BTN_JOYSTICK)
			return SRC_MOUSE;

		if (ring_rsp->code == BTN_TOUCH)
			return SRC_MOUSE;

		return SRC_KEYBOARD;

	default:
		/* We do not use other types */
		return SRC_UNKNOWN;
	}
}

static void *vinput_key_thread(void *args)
{
	vinput_response_t *ring_rsp;
	vinput_priv_t *vinput_priv;

	vinput_priv = (vinput_priv_t *) dev_get_drvdata(vdev_console->dev);
	BUG_ON(!vinput_priv);

	/* Always perform a wait on the completion since we always get an interrupt
	 * per byte (hence a complete will be aised up).
	 */

	while (atomic_read(&vinput_priv->running)) {
		vdevfront_processing_begin(vdev_console);

		while ((ring_rsp = vinput_get_ring_response(&vinput_priv->vinput.ring)) == NULL) {
			vdevfront_processing_end(vdev_console);

			wait_for_completion(&vinput_priv->reader_wait);

			if (!atomic_read(&vinput_priv->running))
				return NULL;

			vdevfront_processing_begin(vdev_console);
		}

		vdevfront_processing_end(vdev_console);

		switch (get_source(ring_rsp)) {
		case SRC_MOUSE:
			soo_mse_event(ring_rsp->type, ring_rsp->code, ring_rsp->value);
			break;

		case SRC_KEYBOARD:
			soo_input_event(ring_rsp->type, ring_rsp->code, ring_rsp->value);
			break;

		default:
			DBG("[vinput] Unknown input source for %d - %d\n", ring_rsp->type, ring_rsp->code);
			break;
		}
	}

	return NULL;
}

static void vinput_probe(struct vbus_device *vdev)
{
	unsigned int evtchn;
	vinput_sring_t *sring;
	struct vbus_transaction vbt;
	vinput_priv_t *vinput_priv;

	DBG0("[vinput] Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return;

	vinput_priv = dev_get_drvdata(vdev->dev);

	/* Local instance */
	vdev_console = vdev;

	init_completion(&vinput_priv->reader_wait);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vinput_priv->vinput.ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	vbus_alloc_evtchn(vdev, &evtchn);

	vinput_priv->vinput.irq = bind_evtchn_to_irq_handler(evtchn, vinput_interrupt, NULL, vdev);
	vinput_priv->vinput.evtchn = evtchn;

	/* Allocate a shared page for the ring */
	sring = (vinput_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&vinput_priv->vinput.ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	vinput_priv->vinput.ring_ref = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vinput_priv->vinput.ring.sring)));

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vinput_priv->vinput.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vinput_priv->vinput.evtchn);

	vbus_transaction_end(vbt);

	vinput_priv->reader_thread = kernel_thread(vinput_key_thread, "vinput_reader_thread", NULL, 0);
}

/* At this point, the FE is not connected. */
static void vinput_reconfiguring(struct vbus_device *vdev)
{
	int res;
	struct vbus_transaction vbt;
	vinput_priv_t *vinput_priv = dev_get_drvdata(vdev->dev);

	DBG0("[vinput] Frontend reconfiguring\n");
	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access(vinput_priv->vinput.ring_ref);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vinput_priv->vinput.ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(vinput_priv->vinput.ring.sring);
	FRONT_RING_INIT(&vinput_priv->vinput.ring, vinput_priv->vinput.ring.sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vinput_priv->vinput.ring.sring)));
	if (res < 0)
		BUG();

	vinput_priv->vinput.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vinput_priv->vinput.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vinput_priv->vinput.evtchn);

	vbus_transaction_end(vbt);
}

static void vinput_shutdown(struct vbus_device *vdev)
{
	DBG0("[vinput] Frontend shutdown\n");
}

static void vinput_closed(struct vbus_device *vdev)
{
	vinput_priv_t *vinput_priv = dev_get_drvdata(vdev->dev);

	DBG0("[vinput] Frontend close\n");

	/**
	 * Free the ring and deallocate the proper data.
	 */

	/* Free resources associated with old device channel. */
	if (vinput_priv->vinput.ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(vinput_priv->vinput.ring_ref);
		free_vpage((addr_t) vinput_priv->vinput.ring.sring);

		vinput_priv->vinput.ring_ref = GRANT_INVALID_REF;
		vinput_priv->vinput.ring.sring = NULL;
	}

	if (vinput_priv->vinput.irq)
		unbind_from_irqhandler(vinput_priv->vinput.irq);

	vinput_priv->vinput.irq = 0;
}

static void vinput_suspend(struct vbus_device *vdev)
{
	DBG0("[vinput] Frontend suspend\n");
}

static void vinput_resume(struct vbus_device *vdev)
{
	DBG0("[vinput] Frontend resume\n");
}

static void vinput_connected(struct vbus_device *vdev)
{
	DBG0("[vinput] Frontend connected\n");
}

vdrvfront_t vinputdrv = { .probe = vinput_probe,
			 .reconfiguring = vinput_reconfiguring,
			 .shutdown = vinput_shutdown,
			 .closed = vinput_closed,
			 .suspend = vinput_suspend,
			 .resume = vinput_resume,
			 .connected = vinput_connected };

static int vinput_init(dev_t *dev, int fdt_offset)
{
	vinput_priv_t *vinput_priv;

	vinput_priv = malloc(sizeof(vinput_priv_t));
	BUG_ON(!vinput_priv);

	memset(vinput_priv, 0, sizeof(vinput_priv_t));

	dev_set_drvdata(dev, vinput_priv);

	vdevfront_init(VINPUT_NAME, &vinputdrv);

	atomic_set(&vinput_priv->running, 1);
	vinput_priv->reader_thread = NULL;

	return 0;
}

REGISTER_DRIVER_POSTCORE("vinput,frontend", vinput_init);
