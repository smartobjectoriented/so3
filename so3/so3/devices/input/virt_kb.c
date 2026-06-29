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

#define GET_KEY 0

const struct ps2_key NO_KEY = { .value = 0, .state = 0 };

static int ioctl_keyboard(int fd, unsigned long cmd, unsigned long args);

struct file_operations virt_kb_fops = { .ioctl = ioctl_keyboard };

struct devclass virt_kb_dev = {
	.class = DEV_CLASS_KEYBOARD,
	.type = VFS_TYPE_DEV_INPUT,
	.fops = &virt_kb_fops,
};

int ioctl_keyboard(int fd, unsigned long cmd, unsigned long args)
{
	switch (cmd) {
	case GET_KEY:
		*((struct ps2_key *) args) = NO_KEY;
		break;

	default:
		/* Unknown command. */
		return -1;
	}

	return 0;
}
static int virt_kb_init(dev_t *dev, int fdt_offset)
{
	devclass_register(dev, &virt_kb_dev);
	return 0;
}

REGISTER_DRIVER_POSTCORE("virt,keyboard", virt_kb_init);
