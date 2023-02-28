/*
 * Copyright (C) 2020 Nikolaos Garanis <nikolaos.garanis@heig-vd.ch>
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

#include <vfs.h>
#include <common.h>
#include <process.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <device/driver.h>
#include <device/fb/soo_fb_fb.h>


void *fb_mmap(int fd, uint32_t virt_addr, uint32_t page_count);
int fb_ioctl(int fd, unsigned long cmd, unsigned long args);

struct file_operations vfb_fops = {
	.mmap = fb_mmap,
	.ioctl = fb_ioctl
};

struct devclass vfb_cdev = {
	.class = DEV_CLASS_FB,
	.type = VFS_TYPE_DEV_FB,
	.fops = &vfb_fops,
};

/* Framebuffer's physical address */
static uint32_t fb_base = 0;

/* Framebuffer's resolution */
static uint32_t fb_hres;
static uint32_t fb_vres;

int fb_init(dev_t *dev)
{
	/* Register the framebuffer so it can be accessed from user space. */
	devclass_register(dev, &vfb_cdev);
	return 0;
}

void *fb_mmap(int fd, uint32_t virt_addr, uint32_t page_count)
{
	uint32_t i;
	pcb_t *pcb = current()->pcb;

	BUG_ON(!fb_base);

	for (i = 0; i < page_count; i++) {
		/* Map the process' pages to physical ones. */
		create_mapping(pcb->pgtable, virt_addr + (i * PAGE_SIZE), fb_base + i * PAGE_SIZE, PAGE_SIZE, false, false);
	}

	return (void *) virt_addr;
}

int fb_ioctl(int fd, unsigned long cmd, unsigned long args)
{
	switch (cmd) {

	case IOCTL_HRES:
		*((uint32_t *) args) = fb_hres;
		return 0;

	case IOCTL_VRES:
		*((uint32_t *) args) = fb_vres;
		return 0;

	case IOCTL_SIZE:
		*((uint32_t *) args) = fb_hres * fb_vres * 4; /* assume 24bpp */
		return 0;

	default:
		/* Unknown command. */
		return -1;
	}
}

void soo_fb_set_info(uint32_t base, uint32_t hres, uint32_t vres)
{
	fb_base = base;
	fb_hres = hres;
	fb_vres = vres;
}

REGISTER_DRIVER_POSTCORE("fb,soo_fb", fb_init);
