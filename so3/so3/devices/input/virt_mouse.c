
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

#include <vfs.h>
#include <common.h>
#include <memory.h>

#include <asm/io.h>
#include <device/driver.h>

#include <device/input/ps2.h>

const struct ps2_mouse EMPTY_STATE = { .x = 0, .y = 0, .left = 0, .right = 0, .middle = 0 };

#define GET_STATE 0
#define SET_SIZE 1

static int ioctl_mouse(int fd, unsigned long cmd, unsigned long args)
{
	switch (cmd) {
	case GET_STATE:
		*((struct ps2_mouse *) args) = EMPTY_STATE;
		break;
	case SET_SIZE:
		break;

	default:
		/* Unknown command. */
		return -1;
	}
	return 0;
}

struct file_operations virt_mouse_fops = { .ioctl = ioctl_mouse };

struct devclass virt_mouse_dev = {
	.class = DEV_CLASS_MOUSE,
	.type = VFS_TYPE_DEV_INPUT,
	.fops = &virt_mouse_fops,
};

static int virt_mouse_init(dev_t *dev, int fdt_offset)
{
	devclass_register(dev, &virt_mouse_dev);
	return 0;
}

REGISTER_DRIVER_POSTCORE("virt,mouse", virt_mouse_init);
