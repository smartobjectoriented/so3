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

/*
 * This file allows for multiple framebuffer devices to register themselves
 * (currently they register only their fops).
 * This file will also register the framebuffer device class with the VFS so
 * that the VFS can retrieve the fops of a registered framebuffer device.
 */

#include <vfs.h>
#include <mutex.h>
#include <device/fb.h>


struct file_operations *get_fb_fops(uint32_t dev_id);

struct mutex fb_lock;

/* All registered framebuffer devices. We only register their fops. */
static struct file_operations *registered_fb[MAX_FB];

/* The device class representing framebuffer devices. */
static struct dev_class fb_dev_class = {
	.name = "fb",
	.get_fops = get_fb_fops
};


/* Return the fops of the framebuffer with the given device id. */
struct file_operations *get_fb_fops(uint32_t id)
{
	if (id < MAX_FB) {
		return registered_fb[id];
	}

	return NULL;
}

/* Registers a framebuffer device by setting its fops. */
int register_fb(struct file_operations *fb_ops)
{
	uint32_t id = 0;

	mutex_lock(&fb_lock);
	while (id < MAX_FB && registered_fb[id]) {
		id++;
	}

	if (id == MAX_FB) {
		mutex_unlock(&fb_lock);
		return -1;
	}

	registered_fb[id] = fb_ops;
	mutex_unlock(&fb_lock);

	return 0;
}

void fb_init(void)
{
	if (vfs_register_dev_class(&fb_dev_class)) {
		printk("%s: fb device class not registered.\n", __func__);
	}
}
