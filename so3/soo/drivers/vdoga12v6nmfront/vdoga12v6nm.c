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

#include <soo/dev/vdoga12v6nm.h>

typedef struct {

	/* Must be the first field */
	vdoga12v6nm_t vdoga12v6nm;

	completion_t cmd_completion;
	int cmd_ret;

	vdoga_interrupt_t __up_interrupt, __down_interrupt;

} vdoga12v6nm_priv_t;

static struct vbus_device *vdoga12v6nm_dev = NULL;

/**
 * Process pending responses in the cmd ring.
 */
static void process_pending_cmd_rsp(struct vbus_device *vdev) {
	vdoga12v6nm_priv_t *vdoga12v6nm_priv = dev_get_drvdata(vdev->dev);
	vdoga12v6nm_response_t *ring_rsp;

	while ((ring_rsp = vdoga12v6nm_get_ring_response(&vdoga12v6nm_priv->vdoga12v6nm.cmd_ring)) != NULL) {

		DBG("%s, cons=%d\n", __func__, i);

		DBG("Ret=%d\n", ring_rsp->ret);
		vdoga12v6nm_priv->cmd_ret = ring_rsp->ret;

		complete(&vdoga12v6nm_priv->cmd_completion);
	}
}

/**
 * Process a command return value coming from the backend.
 */
irq_return_t vdoga12v6nm_cmd_interrupt(int irq, void *dev_id) {
	struct vbus_device *vdev = (struct vbus_device *) dev_id;

	if (vdev->state != VbusStateConnected)
		return IRQ_COMPLETED;

	process_pending_cmd_rsp(vdev);

	return IRQ_COMPLETED;
}

/**
 * Process an up mechanical stop event.
 */
irq_return_t vdoga12v6nm_up_interrupt(int irq, void *dev_id) {
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	vdoga12v6nm_priv_t *vdoga12v6nm_priv = dev_get_drvdata(vdev->dev);

	if (vdoga12v6nm_priv->__up_interrupt)
		(*vdoga12v6nm_priv->__up_interrupt)();

	return IRQ_COMPLETED;
}

/**
 * Process a down mechanical stop event.
 */
irq_return_t vdoga12v6nm_down_interrupt(int irq, void *dev_id) {
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	vdoga12v6nm_priv_t *vdoga12v6nm_priv = dev_get_drvdata(vdev->dev);

	if (vdoga12v6nm_priv->__down_interrupt)
		(*vdoga12v6nm_priv->__down_interrupt)();

	return IRQ_COMPLETED;
}

/**
 * Generate a command request.
 */
static void do_cmd(uint32_t cmd, uint32_t arg) {
	vdoga12v6nm_request_t *ring_req;
	vdoga12v6nm_priv_t *vdoga12v6nm_priv = dev_get_drvdata(vdoga12v6nm_dev->dev);

	DBG(VDOGA12V6NM_PREFIX "0x%08x\n", cmd);

	switch (cmd) {
	case VDOGA12V6NM_ENABLE:
	case VDOGA12V6NM_DISABLE:
	case VDOGA12V6NM_SET_PERCENTAGE_SPEED:
	case VDOGA12V6NM_SET_ROTATION_DIRECTION:
		break;

	default:
		BUG();
	}

	vdevfront_processing_begin(vdoga12v6nm_dev);

	ring_req = vdoga12v6nm_new_ring_request(&vdoga12v6nm_priv->vdoga12v6nm.cmd_ring);

	ring_req->cmd = cmd;
	ring_req->arg = arg;

	vdoga12v6nm_ring_request_ready(&vdoga12v6nm_priv->vdoga12v6nm.cmd_ring);

	notify_remote_via_virq(vdoga12v6nm_priv->vdoga12v6nm.cmd_irq);

	vdevfront_processing_end(vdoga12v6nm_dev);
}

/**
 * Enable the motor.
 */
void vdoga12v6nm_enable(void) {
	vdoga12v6nm_priv_t *vdoga12v6nm_priv = dev_get_drvdata(vdoga12v6nm_dev->dev);

	do_cmd(VDOGA12V6NM_ENABLE, 0);

	/* Wait for the command to be processed by the agency's side */
	wait_for_completion(&vdoga12v6nm_priv->cmd_completion);
}

/**
 * Disable the motor.
 */
void vdoga12v6nm_disable(void) {
	vdoga12v6nm_priv_t *vdoga12v6nm_priv = dev_get_drvdata(vdoga12v6nm_dev->dev);

	do_cmd(VDOGA12V6NM_DISABLE, 0);

	/* Wait for the command to be processed by the agency's side */
	wait_for_completion(&vdoga12v6nm_priv->cmd_completion);
}

/**
 * Set the motor's speed, expressed in percentage.
 */
void vdoga12v6nm_set_percentage_speed(uint32_t speed) {
	vdoga12v6nm_priv_t *vdoga12v6nm_priv = dev_get_drvdata(vdoga12v6nm_dev->dev);

	do_cmd(VDOGA12V6NM_SET_PERCENTAGE_SPEED, speed);

	/* Wait for the command to be processed by the agency's side */
	wait_for_completion(&vdoga12v6nm_priv->cmd_completion);
}

/**
 * Set the motor's rotation direction.
 */
void vdoga12v6nm_set_rotation_direction(uint8_t direction) {
	vdoga12v6nm_priv_t *vdoga12v6nm_priv = dev_get_drvdata(vdoga12v6nm_dev->dev);

	do_cmd(VDOGA12V6NM_SET_ROTATION_DIRECTION, direction);

	/* Wait for the command to be processed by the agency's side */
	wait_for_completion(&vdoga12v6nm_priv->cmd_completion);
}

void vdoga12v6nm_register_interrupts(vdoga_interrupt_t up, vdoga_interrupt_t down) {
	vdoga12v6nm_priv_t *vdoga12v6nm_priv;
	vdoga12v6nm_priv = dev_get_drvdata(vdoga12v6nm_dev->dev);

	if (vdoga12v6nm_dev != NULL) {
		vdoga12v6nm_priv->__up_interrupt = up;
		vdoga12v6nm_priv->__down_interrupt = down;
	}
}

static void vdoga12v6nm_probe(struct vbus_device *vdev) {
	unsigned int evtchn;
	vdoga12v6nm_sring_t *sring;
	struct vbus_transaction vbt;
	vdoga12v6nm_priv_t *vdoga12v6nm_priv;

	DBG0("[" vdoga12v6nm_NAME "] Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return ;

	vdoga12v6nm_priv = dev_get_drvdata(vdev->dev);

	vdoga12v6nm_dev = vdev;

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vdoga12v6nm_priv->vdoga12v6nm.cmd_ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	vbus_alloc_evtchn(vdev, &evtchn);

	vdoga12v6nm_priv->vdoga12v6nm.cmd_irq = bind_evtchn_to_irq_handler(evtchn, vdoga12v6nm_cmd_interrupt, NULL, vdev);
	vdoga12v6nm_priv->vdoga12v6nm.cmd_evtchn = evtchn;

	vbus_alloc_evtchn(vdev, &evtchn);

	vdoga12v6nm_priv->vdoga12v6nm.down_irq = bind_evtchn_to_irq_handler(evtchn, vdoga12v6nm_down_interrupt, NULL, vdev);
	vdoga12v6nm_priv->vdoga12v6nm.down_evtchn = evtchn;

	vbus_alloc_evtchn(vdev, &evtchn);

	vdoga12v6nm_priv->vdoga12v6nm.up_irq = bind_evtchn_to_irq_handler(evtchn, vdoga12v6nm_up_interrupt, NULL, vdev);
	vdoga12v6nm_priv->vdoga12v6nm.up_evtchn = evtchn;


	/* Allocate a shared page for the ring */
	sring = (vdoga12v6nm_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&vdoga12v6nm_priv->vdoga12v6nm.cmd_ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	vdoga12v6nm_priv->vdoga12v6nm.cmd_ring_ref = vbus_grant_ring(vdev,
			phys_to_pfn(virt_to_phys_pt((addr_t) vdoga12v6nm_priv->vdoga12v6nm.cmd_ring.sring)));

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vdoga12v6nm_priv->vdoga12v6nm.cmd_ring_ref);

	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vdoga12v6nm_priv->vdoga12v6nm.cmd_evtchn);
	vbus_printf(vbt, vdev->nodename, "up-evtchn", "%u", vdoga12v6nm_priv->vdoga12v6nm.up_evtchn);
	vbus_printf(vbt, vdev->nodename, "down-evtchn", "%u", vdoga12v6nm_priv->vdoga12v6nm.down_evtchn);

	vbus_transaction_end(vbt);

}

/* At this point, the FE is not connected. */
static void vdoga12v6nm_reconfiguring(struct vbus_device *vdev) {
	int res;
	struct vbus_transaction vbt;
	vdoga12v6nm_priv_t *vdoga12v6nm_priv = dev_get_drvdata(vdev->dev);

	DBG0(VDOGA12V6NM_PREFIX "Frontend reconfiguring\n");

	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access_ref(vdoga12v6nm_priv->vdoga12v6nm.cmd_ring_ref);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vdoga12v6nm_priv->vdoga12v6nm.cmd_ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(vdoga12v6nm_priv->vdoga12v6nm.cmd_ring.sring);
	FRONT_RING_INIT(&vdoga12v6nm_priv->vdoga12v6nm.cmd_ring, (&vdoga12v6nm_priv->vdoga12v6nm.cmd_ring)->sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vdoga12v6nm_priv->vdoga12v6nm.cmd_ring.sring)));
	if (res < 0)
		BUG();

	vdoga12v6nm_priv->vdoga12v6nm.cmd_ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vdoga12v6nm_priv->vdoga12v6nm.cmd_ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vdoga12v6nm_priv->vdoga12v6nm.cmd_evtchn);

	vbus_transaction_end(vbt);
}

static void vdoga12v6nm_shutdown(struct vbus_device *vdev) {

	DBG0(VDOGA12V6NM_PREFIX "Frontend shutdown\n");
}

static void vdoga12v6nm_closed(struct vbus_device *vdev) {
	vdoga12v6nm_priv_t *vdoga12v6nm_priv = dev_get_drvdata(vdev->dev);

	DBG0(VDOGA12V6NM_PREFIX "Frontend close\n");

	/**
	 * Free the ring and deallocate the proper data.
	 */

	/* Free resources associated with old device channel. */
	if (vdoga12v6nm_priv->vdoga12v6nm.cmd_ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(vdoga12v6nm_priv->vdoga12v6nm.cmd_ring_ref);
		free_vpage((addr_t) vdoga12v6nm_priv->vdoga12v6nm.cmd_ring.sring);

		vdoga12v6nm_priv->vdoga12v6nm.cmd_ring_ref = GRANT_INVALID_REF;
		vdoga12v6nm_priv->vdoga12v6nm.cmd_ring.sring = NULL;
	}

	if (vdoga12v6nm_priv->vdoga12v6nm.cmd_irq)
		unbind_from_irqhandler(vdoga12v6nm_priv->vdoga12v6nm.cmd_irq);

	vdoga12v6nm_priv->vdoga12v6nm.cmd_irq = 0;
}

void vdoga12v6nm_suspend(struct vbus_device *vdev) {
	DBG0(VDOGA12V6NM_PREFIX " Frontend suspend\n");
}

void vdoga12v6nm_resume(struct vbus_device *vdev) {
	DBG0(VDOGA12V6NM_PREFIX " Frontend resume\n");

	process_pending_cmd_rsp(vdev);
}

void vdoga12v6nm_connected(struct vbus_device *vdev) {
	vdoga12v6nm_priv_t *vdoga12v6nm_priv = dev_get_drvdata(vdev->dev);

	/* Force the processing of pending requests, if any */
	notify_remote_via_virq(vdoga12v6nm_priv->vdoga12v6nm.cmd_irq);
	notify_remote_via_virq(vdoga12v6nm_priv->vdoga12v6nm.up_irq);
	notify_remote_via_virq(vdoga12v6nm_priv->vdoga12v6nm.down_irq);

	DBG0(VDOGA12V6NM_PREFIX " Frontend connected\n");
}


vdrvfront_t vdoga12v6nmdrv = {
	.probe = vdoga12v6nm_probe,
	.reconfiguring = vdoga12v6nm_reconfiguring,
	.shutdown = vdoga12v6nm_shutdown,
	.closed = vdoga12v6nm_closed,
	.suspend = vdoga12v6nm_suspend,
	.resume = vdoga12v6nm_resume,
	.connected = vdoga12v6nm_connected
};

static int vdoga12v6nm_init(dev_t *dev, int fdt_offset) {
	vdoga12v6nm_priv_t *vdoga12v6nm_priv;

	vdoga12v6nm_priv = malloc(sizeof(vdoga12v6nm_priv_t));
	BUG_ON(!vdoga12v6nm_priv);

	memset(vdoga12v6nm_priv, 0, sizeof(vdoga12v6nm_priv_t));

	dev_set_drvdata(dev, vdoga12v6nm_priv);

	vdevfront_init(VDOGA12V6NM_NAME, &vdoga12v6nmdrv);

	return 0;
}

REGISTER_DRIVER_POSTCORE("vdoga12v6nm,frontend", vdoga12v6nm_init);
