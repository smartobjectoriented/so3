/*
 * Copyright (C) 2025 André Costa <andre_miguel_costa@hotmail.com>
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

#include <process.h>
#include <heap.h>
#include <fb.h>

#include <device/driver.h>

#define BPP 32 /* assume 32bpp */

typedef struct {
	void *vaddr;
	uint32_t hres, vres;
} virtfb_priv_t;

static int fb_ioctl(int fd, unsigned long cmd, unsigned long args)
{
	struct devclass *dev;
	virtfb_priv_t *priv;

	dev = devclass_by_fd(fd);
	BUG_ON(!dev);

	priv = (virtfb_priv_t *) devclass_get_priv(dev);
	BUG_ON(!priv);

	switch (cmd) {
	case IOCTL_FB_HRES:
		*((uint32_t *) args) = priv->hres;
		return 0;

	case IOCTL_FB_VRES:
		*((uint32_t *) args) = priv->vres;
		return 0;

	case IOCTL_FB_SIZE:
		*((uint32_t *) args) = priv->hres * priv->vres * BPP / 8;
		return 0;

	case IOCTL_FB_BPP:
		*((uint32_t *) args) = BPP;
		return 0;

	case IOCTL_FB_IS_REAL:
		*((uint32_t *) args) = 0;
		return 0;

	default:
		/* Unknown command. */
		return -1;
	}
}

static int fb_close(int fd)
{
	struct devclass *dev = devclass_by_fd(fd);
	virtfb_priv_t *priv = (virtfb_priv_t *) devclass_get_priv(dev);

	free(priv->vaddr);

	return 0;
}

struct file_operations virtfb_fops = { .ioctl = fb_ioctl, .close = fb_close };

struct devclass virtfb_cdev = {
	.class = DEV_CLASS_FB,
	.type = VFS_TYPE_DEV_FB,
	.fops = &virtfb_fops,
};

static int virtfb_init(dev_t *dev, int fdt_offset)
{
	int err;
	virtfb_priv_t *priv;

	priv = (virtfb_priv_t *) malloc(sizeof(*priv));
	BUG_ON(!priv);

	err = fdt_property_read_u32(__fdt_addr, fdt_offset, "hres", &priv->hres);
	BUG_ON(err < 0);

	err = fdt_property_read_u32(__fdt_addr, fdt_offset, "vres", &priv->vres);
	BUG_ON(err < 0);

	devclass_register(dev, &virtfb_cdev);
	devclass_set_priv(&virtfb_cdev, priv);

	return 0;
}

REGISTER_DRIVER_POSTCORE("virt,fb", virtfb_init);
