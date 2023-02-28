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
#include <device/fb/so3virt_fb.h>

#include <soo/gnttab.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/console.h>
#include <soo/debug.h>
#include <soo/dev/vfb.h>


void vfb_probe(struct vbus_device *vdev)
{
	int i, res;
	uint32_t fb_base, hres, vres, page_count;
	struct vbus_transaction vbt;
	vfb_t *vfb;
	grant_ref_t fb_ref;
	char dir[40];

	DBG(VFB_PREFIX "Frontend probe\n");
	if (vdev->state == VbusStateConnected)
		return;

	vfb = malloc(sizeof(vfb_t));
	BUG_ON(!vfb);
	memset(vfb, 0, sizeof(vfb_t));
	dev_set_drvdata(vdev->dev, &vfb->vdevfront);

	/* Get display resolution. */
	vbus_transaction_start(&vbt);
	sprintf(dir, "device/%01d/vfb/0/resolution", ME_domID());
	vbus_scanf(vbt, dir, "hor", "%u", &hres);
	vbus_scanf(vbt, dir, "ver", "%u", &vres);
	DBG(VFB_PREFIX "Resolution is %dx%d\n", hres, vres);

	/*
	 * Allocate contiguous memory for the framebuffer and get the physical address.
	 * The pages will be never released. They do not belong to any process.
	 */

	page_count = hres * vres * 4 / PAGE_SIZE; /* assume 24bpp */
	fb_base = get_contig_free_pages(page_count);
	BUG_ON(!fb_base);

	so3virt_fb_set_info(fb_base, hres, vres);

	/* Grant access to every necessary page of the framebuffer. */
	for (i = 0; i < page_count; i++) {
		res = gnttab_grant_foreign_access(vdev->otherend_id, phys_to_pfn(fb_base + i * PAGE_SIZE), 1);
		BUG_ON(res < 0);

		if (i == 0) {
			/* The back-end only needs the first grantref. */
			fb_ref = res;
		}
	}

	DBG(VFB_PREFIX "fb_phys: 0x%08x, fb_ref: 0x%08x\n", fb_base, fb_ref);

	sprintf(dir, "device/%01d/vfb/0/domfb-ref", ME_domID());
	vbus_printf(vbt, dir, "value", "%u", fb_ref);
	vbus_transaction_end(vbt);
}

void vfb_reconfiguring(struct vbus_device *vdev)
{
	DBG(VFB_PREFIX "Frontend reconfiguring\n");
}

void vfb_shutdown(struct vbus_device *vdev)
{
	DBG(VFB_PREFIX "Frontend shutdown\n");
}

void vfb_closed(struct vbus_device *vdev)
{
	DBG(VFB_PREFIX "Frontend close\n");
}

void vfb_suspend(struct vbus_device *vdev)
{
	DBG(VFB_PREFIX "Frontend suspend\n");
}

void vfb_resume(struct vbus_device *vdev)
{
	DBG(VFB_PREFIX "Frontend resume\n");
}

void vfb_connected(struct vbus_device *vdev)
{
	DBG(VFB_PREFIX "Frontend connected\n");
}

vdrvfront_t vfbdrv = {
	.probe = vfb_probe,
	.reconfiguring = vfb_reconfiguring,
	.shutdown = vfb_shutdown,
	.closed = vfb_closed,
	.suspend = vfb_suspend,
	.resume = vfb_resume,
	.connected = vfb_connected
};

static int vfb_init(dev_t *dev)
{
	vdevfront_init(VFB_NAME, &vfbdrv);
	return 0;
}

REGISTER_DRIVER_POSTCORE("vfb,frontend", vfb_init);
