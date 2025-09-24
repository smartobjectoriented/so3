/*
 * Copyright (C) 2025 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
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
#include <soo/debug.h>

#include <soo/dev/vlogs.h>

static struct vbus_device *vlogs_dev = NULL;

typedef struct {
	/* Must be the first field */
	vlogs_t vlogs;

	completion_t reader_wait;

} vlogs_priv_t;

static irq_return_t vlogs_interrupt(int irq, void *dev_id)
{
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	vlogs_priv_t *vlogs_priv = (vlogs_priv_t *) dev_get_drvdata(vdev->dev);

	complete(&vlogs_priv->reader_wait);

	return IRQ_COMPLETED;
}

/*
 * Can be used outside the frontend by other subsystems.
 */
bool vlogs_ready(void)
{
	return (vlogs_dev && (vlogs_dev->state == VbusStateConnected));
}

/**
 * Send a string on the vlogs device.
 */
void vlogs_write(const char *fmt, ...)
{
	static char buf[1024];
	va_list args;
	vlogs_request_t *ring_req;
	vlogs_priv_t *vlogs_priv;

	if (!vlogs_dev)
		return;

	va_start(args, fmt);
	(void) vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	vlogs_priv = (vlogs_priv_t *) dev_get_drvdata(vlogs_dev->dev);
	BUG_ON(!vlogs_priv);

	vdevfront_processing_begin(vlogs_dev);

	ring_req = vlogs_new_ring_request(&vlogs_priv->vlogs.ring);
	strcpy(ring_req->log, buf);

	vlogs_ring_request_ready(&vlogs_priv->vlogs.ring);

	notify_remote_via_virq(vlogs_priv->vlogs.irq);

	vdevfront_processing_end(vlogs_dev);
}

static void vlogs_probe(struct vbus_device *vdev)
{
	unsigned int evtchn;
	vlogs_sring_t *sring;
	struct vbus_transaction vbt;
	vlogs_priv_t *vlogs_priv;

	DBG0("[vlogs] Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return;

	vlogs_priv = dev_get_drvdata(vdev->dev);

	/* Local instance */
	vlogs_dev = vdev;

	init_completion(&vlogs_priv->reader_wait);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vlogs_priv->vlogs.ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	vbus_alloc_evtchn(vdev, &evtchn);

	vlogs_priv->vlogs.irq = bind_evtchn_to_irq_handler(evtchn, vlogs_interrupt, NULL, vdev);
	vlogs_priv->vlogs.evtchn = evtchn;

	/* Allocate a shared page for the ring */
	sring = (vlogs_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&vlogs_priv->vlogs.ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	vlogs_priv->vlogs.ring_ref = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vlogs_priv->vlogs.ring.sring)));

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vlogs_priv->vlogs.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vlogs_priv->vlogs.evtchn);

	vbus_transaction_end(vbt);
}

/* At this point, the FE is not connected. */
static void vlogs_reconfiguring(struct vbus_device *vdev)
{
	int res;
	struct vbus_transaction vbt;
	vlogs_priv_t *vlogs_priv = dev_get_drvdata(vdev->dev);

	DBG0("[vlogs] Frontend reconfiguring\n");
	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access(vlogs_priv->vlogs.ring_ref);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vlogs_priv->vlogs.ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(vlogs_priv->vlogs.ring.sring);
	FRONT_RING_INIT(&vlogs_priv->vlogs.ring, vlogs_priv->vlogs.ring.sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vlogs_priv->vlogs.ring.sring)));
	if (res < 0)
		BUG();

	vlogs_priv->vlogs.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vlogs_priv->vlogs.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vlogs_priv->vlogs.evtchn);

	vbus_transaction_end(vbt);
}

static void vlogs_shutdown(struct vbus_device *vdev)
{
	DBG0("[vlogs] Frontend shutdown\n");
}

static void vlogs_closed(struct vbus_device *vdev)
{
	vlogs_priv_t *vlogs_priv = dev_get_drvdata(vdev->dev);

	DBG0("[vlogs] Frontend close\n");

	/**
	 * Free the ring and deallocate the proper data.
	 */

	/* Free resources associated with old device channel. */
	if (vlogs_priv->vlogs.ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(vlogs_priv->vlogs.ring_ref);
		free_vpage((addr_t) vlogs_priv->vlogs.ring.sring);

		vlogs_priv->vlogs.ring_ref = GRANT_INVALID_REF;
		vlogs_priv->vlogs.ring.sring = NULL;
	}

	if (vlogs_priv->vlogs.irq)
		unbind_from_irqhandler(vlogs_priv->vlogs.irq);

	vlogs_priv->vlogs.irq = 0;
}

static void vlogs_suspend(struct vbus_device *vdev)
{
	DBG0("[vlogs] Frontend suspend\n");
}

static void vlogs_resume(struct vbus_device *vdev)
{
	DBG0("[vlogs] Frontend resume\n");
}

static void vlogs_connected(struct vbus_device *vdev)
{
	DBG0("[vlogs] Frontend connected\n");
}

vdrvfront_t vlogsdrv = { .probe = vlogs_probe,
			 .reconfiguring = vlogs_reconfiguring,
			 .shutdown = vlogs_shutdown,
			 .closed = vlogs_closed,
			 .suspend = vlogs_suspend,
			 .resume = vlogs_resume,
			 .connected = vlogs_connected };

/* Cdev - Write function (called from user-space) */
static int vlogs_cwrite(int fd, const void *buffer, int count)
{
	static char msg[1024];

	sprintf(msg, "[ME:%d] %s", get_ME_desc()->slotID, buffer);

	vlogs_write((char *) msg);

	return count;
}

struct file_operations vlogs_fops = {
	.write = vlogs_cwrite,
};

struct devclass vlogs_cdev = {
	.class = "capsule_logs",
	.type = VFS_TYPE_DEV_CHAR,
	.fops = &vlogs_fops,
};

static int vlogs_init(dev_t *dev, int fdt_offset)
{
	vlogs_priv_t *vlogs_priv;

	/* Register the mydev driver so it can be accessed from user space. */
	devclass_register(dev, &vlogs_cdev);

	vlogs_priv = malloc(sizeof(vlogs_priv_t));
	BUG_ON(!vlogs_priv);

	memset(vlogs_priv, 0, sizeof(vlogs_priv_t));

	dev_set_drvdata(dev, vlogs_priv);

	vdevfront_init(VLOGS_NAME, &vlogsdrv);

	return 0;
}

REGISTER_DRIVER_POSTCORE("vlogs,frontend", vlogs_init);
