/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

/* Simple of example of devclass device */

#include <vfs.h>

#include <device/driver.h>

char internal_buffer[20];

static int mydev_write(int fd, const void *buffer, int count) {

	strcpy(internal_buffer, buffer);

	return count;
}

static int mydev_read(int fd, void *buffer, int count) {

	strcpy(buffer, internal_buffer);

	return strlen(internal_buffer)+1;
}

struct file_operations mydev_fops = {
	.write = mydev_write,
	.read = mydev_read
};

struct devclass mydev_dev = {
	.class = "mydev",
	.type = VFS_TYPE_DEV_CHAR,
	.fops = &mydev_fops,
};


int mydev_init(dev_t *dev, int fdt_offset) {
	int node;
	const char *propname;

	/* Register the mydev driver so it can be accessed from user space. */
	devclass_register(dev, &mydev_dev);

	node = fdt_find_node_by_name(__fdt_addr, 0, "mydev");
	fdt_property_read_string(__fdt_addr, node, "compatible", &propname);

	return 0;
}


REGISTER_DRIVER_POSTCORE("arm,mydev", mydev_init);
