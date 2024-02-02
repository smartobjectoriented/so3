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

#include <soo/dev/vknx.h>

typedef struct {

	/* Must be the first field */
	vknx_t vknx;
	vknx_response_t *resp;

	struct completion waitlock;

} vknx_priv_t;

static struct vbus_device *vknx_dev = NULL;

/**
 * @brief  Print a datapoint array. Use for debug purposes.  
 * @param  dps: Array of datapoints 
 * @param  dp_count: Number of datapoints in the array 
 * @retval None
 */
void vknx_print_dps(dp_t *dps, int dp_count) {
	int i, j;

	printk("Datapoints:\n");
	for (i = 0; i < dp_count; i++) {
		printk("\n");
		printk("ID: 0x%04X\n", dps[i].id);
		printk("State/cmd: 0x%02X\n", dps[i].cmd);
		printk("Data length: %d\n", dps[i].data_len);
		printk("Data:\n");
		for (j = 0; j < dps[i].data_len; j++) {
			printk("[%d]: 0x%02X\n", j, dps[i].data[j]);
		}
		printk("\n");
	}
}

int get_knx_data(vknx_response_t *data) {
	vknx_priv_t *vknx_priv = dev_get_drvdata(vknx_dev->dev);
	vknx_response_t *ring_rsp;
	int ret = 0;
	
	wait_for_completion(&vknx_priv->waitlock);
	
	vdevfront_processing_begin(vknx_dev);
	if ((ring_rsp = vknx_get_ring_response(&vknx_priv->vknx.ring)) != NULL) {
		memcpy(data, ring_rsp, sizeof(vknx_response_t));
	} else 
		ret = -1;
	vdevfront_processing_end(vknx_dev);

	return ret;
}

void vknx_get_dp_value(uint16_t first_dp, int dp_count) {
	vknx_priv_t *vknx_priv = dev_get_drvdata(vknx_dev->dev);
	vknx_request_t *ring_req;

	vdevfront_processing_begin(vknx_dev);

	if (!RING_REQ_FULL(&vknx_priv->vknx.ring)) {
		DBG("%s, %d\n", __func__, ME_domID());
		ring_req = vknx_new_ring_request(&vknx_priv->vknx.ring);
		ring_req->dp_count = dp_count;
		ring_req->type = GET_DP_VALUE;
		ring_req->datapoints[0].id = first_dp;

		vknx_ring_request_ready(&vknx_priv->vknx.ring);
		notify_remote_via_virq(vknx_priv->vknx.irq);

	} else {
		DBG("Ring full\n");
		BUG();
	}

	vdevfront_processing_end(vknx_dev);
}

void vknx_set_dp_value(dp_t* datapoints, int dp_count) {
	vknx_priv_t *vknx_priv = dev_get_drvdata(vknx_dev->dev);
	vknx_request_t *ring_req;
	int i;

	vdevfront_processing_begin(vknx_dev);

	if (!RING_REQ_FULL(&vknx_priv->vknx.ring)) {
		DBG("%s, %d\n", __func__, ME_domID());
		ring_req = vknx_new_ring_request(&vknx_priv->vknx.ring);
		ring_req->dp_count = dp_count;
		ring_req->type = SET_DP_VALUE;

		for (i = 0; i < dp_count; i++)
			memcpy(&ring_req->datapoints[i], &datapoints[i], sizeof(dp_t));

		vknx_ring_request_ready(&vknx_priv->vknx.ring);
		notify_remote_via_virq(vknx_priv->vknx.irq);

	} else {
		DBG("Ring full\n");
		BUG();
	}

	vdevfront_processing_end(vknx_dev);
}

irq_return_t vknx_interrupt(int irq, void *dev_id) {
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	vknx_priv_t *vknx_priv = dev_get_drvdata(vdev->dev);

	DBG(VKNX_PREFIX "%s, %d\n", __func__, ME_domID());
	complete(&vknx_priv->waitlock);

	return IRQ_COMPLETED;
}

static void vknx_probe(struct vbus_device *vdev) {
	int res;
	unsigned int evtchn;
	vknx_sring_t *sring;
	struct vbus_transaction vbt;
	vknx_priv_t *vknx_priv;

	DBG0(VKNX_PREFIX " Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return ;

	vknx_priv = dev_get_drvdata(vdev->dev);
	vknx_dev = vdev;

	DBG(VKNX_PREFIX "Setup ring\n");
	init_completion(&vknx_priv->waitlock);

	/* Prepare to set up the ring. */

	vknx_priv->vknx.ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	vbus_alloc_evtchn(vdev, &evtchn);

	res = bind_evtchn_to_irq_handler(evtchn, vknx_interrupt, NULL, vdev);
	if (res <= 0) {
		lprintk("%s - line %d: Binding event channel failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	vknx_priv->vknx.evtchn = evtchn;
	vknx_priv->vknx.irq = res;

	/* Allocate a shared page for the ring */
	sring = (vknx_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&vknx_priv->vknx.ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vknx_priv->vknx.ring.sring)));
	if (res < 0)
		BUG();

	vknx_priv->vknx.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vknx_priv->vknx.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vknx_priv->vknx.evtchn);

	vbus_transaction_end(vbt);

	DBG(VKNX_PREFIX " Frontend probed successfully\n");
}

/* At this point, the FE is not connected. */
static void vknx_reconfiguring(struct vbus_device *vdev) {
	int res;
	struct vbus_transaction vbt;
	vknx_priv_t *vknx_priv = dev_get_drvdata(vdev->dev);

	DBG0(VKNX_PREFIX " Frontend reconfiguring\n");
	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access_ref(vknx_priv->vknx.ring_ref);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vknx_priv->vknx.ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(vknx_priv->vknx.ring.sring);
	FRONT_RING_INIT(&vknx_priv->vknx.ring, vknx_priv->vknx.ring.sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vknx_priv->vknx.ring.sring)));
	if (res < 0)
		BUG();

	vknx_priv->vknx.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vknx_priv->vknx.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vknx_priv->vknx.evtchn);

	vbus_transaction_end(vbt);
}

static void vknx_shutdown(struct vbus_device *vdev) {

	DBG0(VKNX_PREFIX " Frontend shutdown\n");
}

static void vknx_closed(struct vbus_device *vdev) {
	vknx_priv_t *vknx_priv = dev_get_drvdata(vdev->dev);

	DBG0(VKNX_PREFIX " close\n");

	/**
	 * Free the ring and deallocate the proper data.
	 */

	/* Free resources associated with old device channel. */
	if (vknx_priv->vknx.ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(vknx_priv->vknx.ring_ref);
		free_vpage((addr_t) vknx_priv->vknx.ring.sring);

		vknx_priv->vknx.ring_ref = GRANT_INVALID_REF;
		vknx_priv->vknx.ring.sring = NULL;
	}

	if (vknx_priv->vknx.irq)
		unbind_from_irqhandler(vknx_priv->vknx.irq);

	vknx_priv->vknx.irq = 0;

	DBG0(VKNX_PREFIX " close\n");

	free(vknx_priv);

	DBG0(VKNX_PREFIX " closed\n");
}

static void vknx_suspend(struct vbus_device *vdev) {

	DBG0(VKNX_PREFIX " Frontend suspend\n");
}

static void vknx_resume(struct vbus_device *vdev) {

	DBG0(VKNX_PREFIX " Frontend resume\n");
}

static void vknx_connected(struct vbus_device *vdev) {
	vknx_priv_t *vknx_priv = dev_get_drvdata(vdev->dev);

	DBG0(VKNX_PREFIX " Frontend connected\n");

	/* Force the processing of pending requests, if any */
	notify_remote_via_virq(vknx_priv->vknx.irq);
}

vdrvfront_t vknxdrv = {
	.probe = vknx_probe,
	.reconfiguring = vknx_reconfiguring,
	.shutdown = vknx_shutdown,
	.closed = vknx_closed,
	.suspend = vknx_suspend,
	.resume = vknx_resume,
	.connected = vknx_connected
};

static int vknx_init(dev_t *dev, int fdt_offset) {
	vknx_priv_t *vknx_priv;

	vknx_priv = malloc(sizeof(vknx_priv_t));
	BUG_ON(!vknx_priv);

	memset(vknx_priv, 0, sizeof(vknx_priv_t));

	dev_set_drvdata(dev, vknx_priv);

	vdevfront_init(VKNX_NAME, &vknxdrv);

	DBG(VKNX_PREFIX " initialized successfully\n");

	return 0;
}

REGISTER_DRIVER_POSTCORE("vknx,frontend", vknx_init);
