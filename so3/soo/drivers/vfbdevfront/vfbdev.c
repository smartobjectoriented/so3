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

#if 0
#define DEBUG
#endif

#include <heap.h>
#include <mutex.h>
#include <delay.h>
#include <memory.h>
#include <process.h>
#include <fb.h>

#include <asm/mmu.h>

#include <device/driver.h>

#include <soo/evtchn.h>
#include <soo/gnttab.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/console.h>
#include <soo/debug.h>

#include <soo/dev/vfbdev.h>

typedef struct {
	/* Must be the first field */
	vfbdev_t vfbdev;

	completion_t reader_wait;

	uint32_t hres;
	uint32_t vres;
	uint32_t bpp;
	size_t memory_size;
	addr_t fb_paddr;
} vfbdev_priv_t;

/* Our unique vfbdev instance. */
static struct vbus_device *vfbdev_dev = NULL;

irq_return_t vfbdev_interrupt(int irq, void *dev_id)
{
	struct vbus_device *vdev = (struct vbus_device *) dev_id;
	vfbdev_priv_t *vfbdev_priv = (vfbdev_priv_t *) dev_get_drvdata(vdev->dev);

	complete(&vfbdev_priv->reader_wait);

	return IRQ_COMPLETED;
}

void vfbdev_probe(struct vbus_device *vdev)
{
	unsigned int evtchn;
	vfbdev_sring_t *sring;
	struct vbus_transaction vbt;
	vfbdev_priv_t *vfbdev_priv;

	DBG0("[vfbdev] Frontend probe\n");

	if (vdev->state == VbusStateConnected)
		return;

	vfbdev_priv = dev_get_drvdata(vdev->dev);

	/* Local instance */
	vfbdev_dev = vdev;

	init_completion(&vfbdev_priv->reader_wait);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vfbdev_priv->vfbdev.ring_ref = GRANT_INVALID_REF;

	/* Allocate an event channel associated to the ring */
	vbus_alloc_evtchn(vdev, &evtchn);

	vfbdev_priv->vfbdev.irq = bind_evtchn_to_irq_handler(evtchn, vfbdev_interrupt, NULL, vdev);
	vfbdev_priv->vfbdev.evtchn = evtchn;

	/* Allocate a shared page for the ring */
	sring = (vfbdev_sring_t *) get_free_vpage();
	if (!sring) {
		lprintk("%s - line %d: Allocating shared ring failed for device %s\n", __func__, __LINE__, vdev->nodename);
		BUG();
	}

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&vfbdev_priv->vfbdev.ring, sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	vfbdev_priv->vfbdev.ring_ref =
		vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vfbdev_priv->vfbdev.ring.sring)));

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vfbdev_priv->vfbdev.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vfbdev_priv->vfbdev.evtchn);

	vbus_transaction_end(vbt);
}

/* At this point, the FE is not connected. */
void vfbdev_reconfiguring(struct vbus_device *vdev)
{
	int res;
	struct vbus_transaction vbt;
	vfbdev_priv_t *vfbdev_priv = dev_get_drvdata(vdev->dev);

	DBG0("[vfbdev] Frontend reconfiguring\n");
	/* The shared page already exists */
	/* Re-init */

	gnttab_end_foreign_access(vfbdev_priv->vfbdev.ring_ref);

	DBG("Frontend: Setup ring\n");

	/* Prepare to set up the ring. */

	vfbdev_priv->vfbdev.ring_ref = GRANT_INVALID_REF;

	SHARED_RING_INIT(vfbdev_priv->vfbdev.ring.sring);
	FRONT_RING_INIT(&vfbdev_priv->vfbdev.ring, vfbdev_priv->vfbdev.ring.sring, PAGE_SIZE);

	/* Prepare the shared to page to be visible on the other end */

	res = vbus_grant_ring(vdev, phys_to_pfn(virt_to_phys_pt((addr_t) vfbdev_priv->vfbdev.ring.sring)));
	if (res < 0)
		BUG();

	vfbdev_priv->vfbdev.ring_ref = res;

	vbus_transaction_start(&vbt);

	vbus_printf(vbt, vdev->nodename, "ring-ref", "%u", vfbdev_priv->vfbdev.ring_ref);
	vbus_printf(vbt, vdev->nodename, "ring-evtchn", "%u", vfbdev_priv->vfbdev.evtchn);

	vbus_transaction_end(vbt);
}

void vfbdev_shutdown(struct vbus_device *vdev)
{
	DBG0("[vfbdev] Frontend shutdown\n");
}

void vfbdev_closed(struct vbus_device *vdev)
{
	vfbdev_priv_t *vfbdev_priv = dev_get_drvdata(vdev->dev);

	DBG0("[vfbdev] Frontend close\n");

	/**
	 * Free the ring and deallocate the proper data.
	 */

	/* Free resources associated with old device channel. */
	if (vfbdev_priv->vfbdev.ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(vfbdev_priv->vfbdev.ring_ref);
		free_vpage((addr_t) vfbdev_priv->vfbdev.ring.sring);

		vfbdev_priv->vfbdev.ring_ref = GRANT_INVALID_REF;
		vfbdev_priv->vfbdev.ring.sring = NULL;
	}

	if (vfbdev_priv->vfbdev.irq)
		unbind_from_irqhandler(vfbdev_priv->vfbdev.irq);

	vfbdev_priv->vfbdev.irq = 0;
}

void vfbdev_suspend(struct vbus_device *vdev)
{
	DBG0("[vfbdev] Frontend suspend\n");
}

void vfbdev_resume(struct vbus_device *vdev)
{
	DBG0("[vfbdev] Frontend resume\n");
}

void vfbdev_connected(struct vbus_device *vdev)
{
	DBG0("[vfbdev] Frontend connected\n");
}

vdrvfront_t vfbdevdrv = {
	.probe = vfbdev_probe,
	.reconfiguring = vfbdev_reconfiguring,
	.shutdown = vfbdev_shutdown,
	.closed = vfbdev_closed,
	.suspend = vfbdev_suspend,
	.resume = vfbdev_resume,
	.connected = vfbdev_connected,
};

/* Char device associated to the framebuffer */

/**
 * Retrieve data (resolution, size, fb address) from peer and AVZ.
 */
static void retrieve_data(vfbdev_priv_t *priv)
{
	vfbdev_request_t *ring_req;
	vfbdev_response_t *ring_rsp;
	avz_hyp_t hyp_args;

	/* Data have already been retrieved */
	if ((priv->memory_size != 0) && (priv->fb_paddr != 0))
		return;

	/* Retrieve resolution and size from peer */
	vdevfront_processing_begin(vfbdev_dev);

	ring_req = vfbdev_new_ring_request(&priv->vfbdev.ring);
	vfbdev_ring_request_ready(&priv->vfbdev.ring);
	notify_remote_via_virq(priv->vfbdev.irq);
	vdevfront_processing_end(vfbdev_dev);

	vdevfront_processing_begin(vfbdev_dev);

	while ((ring_rsp = vfbdev_get_ring_response(&priv->vfbdev.ring)) == NULL) {
		vdevfront_processing_end(vfbdev_dev);

		wait_for_completion(&priv->reader_wait);

		vdevfront_processing_begin(vfbdev_dev);
	}

	priv->hres = ring_rsp->hres;
	priv->vres = ring_rsp->vres;
	priv->bpp = ring_rsp->bpp;
	priv->memory_size = ring_rsp->memory_size;

	vdevfront_processing_end(vfbdev_dev);

	/* Retrieve fb address from AVZ */
	hyp_args.cmd = AVZ_FBDEV_GET_ME_ADDR;
	avz_hypercall(&hyp_args);
	priv->fb_paddr = hyp_args.u.avz_fbdev_addr_args.paddr;
}

static int vfbdev_mmap(int fd, addr_t virt_addr, uint32_t page_count, off_t offset)
{
	pcb_t *pcb = current()->pcb;
	struct devclass *dev;
	vfbdev_priv_t *priv;

	dev = devclass_by_fd(fd);
	BUG_ON(!dev);
	priv = devclass_get_priv(dev);
	BUG_ON(!priv);

	retrieve_data(priv);

	/* Ensure that we got a framebuffer */
	if ((priv->memory_size == 0) || (priv->fb_paddr == 0)) {
		return -ENODEV;
	}

	/* Ensure requested mapping size doesn't overflow actual framebuffer size */
	if (page_count > (priv->memory_size / PAGE_SIZE)) {
		return -EINVAL;
	}

	create_mapping(pcb->pgtable, virt_addr, priv->fb_paddr, page_count * PAGE_SIZE, true);

	return 0;
}

static int vfbdev_ioctl(int fd, unsigned long cmd, unsigned long args)
{
	struct devclass *dev;
	vfbdev_priv_t *priv;

	dev = devclass_by_fd(fd);
	BUG_ON(!dev);
	priv = devclass_get_priv(dev);
	BUG_ON(!priv);

	retrieve_data(priv);

	/* Set returned value accordingly to the command */
	switch (cmd) {
	case IOCTL_FB_HRES:
		*((uint32_t *) args) = priv->hres;
		return 0;

	case IOCTL_FB_VRES:
		*((uint32_t *) args) = priv->vres;
		return 0;

	case IOCTL_FB_BPP:
		*((uint32_t *) args) = priv->bpp;
		return 0;

	case IOCTL_FB_SIZE:
		*((uint32_t *) args) = priv->memory_size;
		return 0;

	default:
		/* Unknown command. */
		return -EINVAL;
	}
}

static struct file_operations vfbdev_fops = {
	.mmap = vfbdev_mmap,
	.ioctl = vfbdev_ioctl,
};

static struct devclass vfbdev_cdev = {
	.class = DEV_CLASS_FB,
	.type = VFS_TYPE_DEV_FB,
	.fops = &vfbdev_fops,
};

static int vfbdev_init(dev_t *dev, int fdt_offset)
{
	vfbdev_priv_t *vfbdev_priv;

	vfbdev_priv = malloc(sizeof(vfbdev_priv_t));
	BUG_ON(!vfbdev_priv);

	memset(vfbdev_priv, 0, sizeof(vfbdev_priv_t));

	devclass_register(dev, &vfbdev_cdev);
	devclass_set_priv(&vfbdev_cdev, vfbdev_priv);
	dev_set_drvdata(dev, vfbdev_priv);

	vdevfront_init(VFBDEV_NAME, &vfbdevdrv);

	return 0;
}

REGISTER_DRIVER_POSTCORE("vfbdev,frontend", vfbdev_init);
