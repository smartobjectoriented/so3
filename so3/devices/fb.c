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
#include <device/fb.h>

#define MAX_FB 4

struct mutex fb_lock;
static struct file_operations *registered_fb_ops[MAX_FB];

/* Returns the fops associated with the fb device of the given id. */
struct file_operations *get_fb_ops(uint32_t id)
{
	if (id < MAX_FB) {
		return registered_fb_ops[id];
	}

	return NULL;
}

/* Registers a fb device by setting its fops. */
int register_fb_ops(struct file_operations* fb_ops)
{
	uint32_t id = 0;

	mutex_lock(&fb_lock);
	while (id < MAX_FB && registered_fb_ops[id] != NULL) {
		id++;
	}

	mutex_unlock(&fb_lock);

	if (id == MAX_FB) {
		return -1;
	}

	registered_fb_ops[id] = fb_ops;
	return 0;
}
