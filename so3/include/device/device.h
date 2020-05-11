/*
 * Copyright (C) 2015-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef DEVICE_H
#define DEVICE_H

#include <list.h>
#include <vfs.h>

#include <device/fdt/fdt.h>

/* Filename prefix of a device. */
#define DEV_PREFIX      "/dev/"
#define DEV_PREFIX_LEN  (sizeof(DEV_PREFIX)-1)

/* Device classes. */
#define DEV_CLASS_FB    "fb"
#define DEV_CLASS_INPUT "input"

#define INITCALLS_LEVELS 2

/* Device status. */
typedef enum {
	STATUS_UNKNOWN,
	STATUS_DISABLED,
	STATUS_INIT_PENDING,
	STATUS_INITIALIZED,
} dev_status_t;

struct dev {
	char compatible[MAX_COMPAT_SIZE];
	char nodename[MAX_NODE_SIZE];
	uint32_t base;
	uint32_t size;
	int irq;
	dev_status_t status;
	int offset_dts;
	struct dev *parent;
	void *fdt;
};
typedef struct dev dev_t;

/* Structure used by drivers to register their devices. */
struct devclass {
	dev_t *dev; 			/* Reference to the device */
	char *class;			/* device class */

	uint32_t id_start, id_end;	/* Range of device associated to this device */

	uint32_t type;			/* vfs type */
	struct file_operations *fops;	/* the device's fops */

	struct list_head list;

	void *priv;			/* Private data for this device */
};

/*
 * Core drivers are initialized before postcore drivers.
 */
enum inicalls_levels { CORE, POSTCORE };


/*
 * Get device information from a device tree
 * This function will be in charge of allocating dev_inf struct;
 */
int get_dev_info(const void *fdt, int offset, const char *compat, dev_t *info);
int fdt_get_int(dev_t *dev, const char *name);
bool fdt_device_is_available(int node_offset);

void devclass_register(dev_t *dev, struct devclass *);
struct file_operations *devclass_get_fops(const char *filename, uint32_t *vfs_type);
struct devclass *devclass_get_cdev(const char *filename);
int devclass_get_id(int fd);

void devclass_set_priv(struct devclass *cdev, void *priv);
void *devclass_get_priv(struct devclass *cdev);

void devices_init(void);

#endif /* DEVICE_H */
