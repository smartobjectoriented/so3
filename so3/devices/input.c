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

#include <vfs.h>
#include <mutex.h>
#include <device/input.h>


struct file_operations *get_input_fops(uint32_t dev_id);

struct mutex input_lock;

/* All registered input devices. We only register their fops. */
static struct file_operations *registered_input[MAX_INPUT];

/* The device class representing input devices. */
static struct dev_class input_dev_class = {
	.name = "input",
	.get_fops = get_input_fops
};


/* Return the fops of the input device with the given id. */
struct file_operations *get_input_fops(uint32_t id)
{
	if (id < MAX_INPUT) {
		return registered_input[id];
	}

	return NULL;
}

/* Registers an input device by setting its fops. */
int register_input(struct file_operations *fops)
{
	uint32_t id = 0;

	mutex_lock(&input_lock);
	while (id < MAX_INPUT && registered_input[id]) {
		id++;
	}

	if (id == MAX_INPUT) {
		mutex_unlock(&input_lock);
		return -1;
	}

	registered_input[id] = fops;
	mutex_unlock(&input_lock);

	return 0;
}

void input_init(void)
{
	if (vfs_register_dev_class(&input_dev_class)) {
		printk("%s: input device class not registered.\n", __func__);
	}
}
