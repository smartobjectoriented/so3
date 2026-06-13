/*
 * Copyright (C) 2024-2025 André Costa <andre_miguel_costa@hotmail.com>
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

#include <device/device.h>
#include <devfs/devfs.h>
#include <heap.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <vfs.h>

/*
 * devfs handles the opening and reading of the /dev directory
 * We can open it and read the registered devices
 * Instead of reading files, we actually look for the registered devices 
 * thus returning "virtual files"
 */

/* Open the dev directory */
int devfs_open(int fd, const char *filename)
{
	devfs_data *priv;
	priv = malloc(sizeof(devfs_data));
	if (!priv) {
		return -ENOMEM;
	}
	priv->current_devclass = NULL;
	priv->current_devclass_entry_id = 0;
	priv->current_devclass_index = 0;
	vfs_set_priv(fd, priv);
	return 0;
}

/* Close the dev directory */
int devfs_close(int fd)
{
	devfs_data *priv = (devfs_data *) vfs_get_priv(fd);
	if (!priv) {
		return 0;
	}
	free(priv);
	return 0;
}

struct dirent *devfs_readdir(int fd)
{
	struct dirent *dent;
	devfs_data *priv = (devfs_data *) vfs_get_priv(fd);
	bool is_single_entry;

	dent = &priv->dent;

	/* Whenever we are done with a certain devclass entry this pointer is null */
	if (!priv->current_devclass) {
		priv->current_devclass = devclass_get_by_index(priv->current_devclass_index);

		if (!priv->current_devclass) {
			return NULL;
		}
		priv->current_devclass_entry_id = priv->current_devclass->id_start;
	}

	is_single_entry = priv->current_devclass->id_start == priv->current_devclass->id_end;

	/* Special case is when there's only one entry,
	 * we don't actually need to specify the id after the name
	 */
	if (is_single_entry) {
		snprintf(dent->d_name, sizeof(dent->d_name), "%s", priv->current_devclass->class);
	} else {
		snprintf(dent->d_name, sizeof(dent->d_name), "%s%d", priv->current_devclass->class,
			 priv->current_devclass_entry_id);
	}

	/* For now we only support char devices inside /dev */
	dent->d_type = DT_CHR;

	// check if this was the last entry id
	if (is_single_entry || priv->current_devclass_entry_id == priv->current_devclass->id_end) {
		priv->current_devclass_index++;
		/* We are done with this devclass, make it NULL */
		priv->current_devclass = NULL;
	} else {
		/* else keep the same devclass */
		priv->current_devclass_entry_id++;
	}
	return dent;
}
static struct file_operations devfs_ops = {
	.open = devfs_open,
	.close = devfs_close,
	.readdir = devfs_readdir,
};

struct file_operations *register_devfs(void)
{
	return &devfs_ops;
}
