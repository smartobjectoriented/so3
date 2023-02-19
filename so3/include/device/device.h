/*
 * Copyright (C) 2015-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <device/fdt.h>
#include <device/irq.h>

/* Filename prefix of a device. */
#define DEV_PREFIX      "/dev/"
#define DEV_PREFIX_LEN  (sizeof(DEV_PREFIX)-1)

/* Device classes. */
#define DEV_CLASS_FB    	"fb"
#define DEV_CLASS_INPUT 	"input"
#define DEV_CLASS_MOUSE   	"mouse"
#define DEV_CLASS_KEYBOARD 	"keyboard"

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

	dev_status_t status;

	int offset_dts;
	struct dev *parent;
	void *fdt;

	/* Private data regarding the driver for this device. */
	void *driver_data;

	/* Reference for the glbal list of dev_t */
	struct list_head list;
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

static inline void dev_set_drvdata(dev_t *dev, void *data)
{
	dev->driver_data = data;
}

static inline void *dev_get_drvdata(const dev_t *dev)
{
	return dev->driver_data;
}

/*
 * Attach a private data structure to a devclass.
 */
static inline void devclass_set_priv(struct devclass *dev, void *priv) {
	dev->priv = priv;
}

static inline void *devclass_get_priv(struct devclass *dev) {
	return dev->priv;
}

void devclass_register(dev_t *dev, struct devclass *devclass);
struct file_operations *devclass_get_fops(const char *filename, uint32_t *vfs_type);

struct devclass *devclass_by_filename(const char *filename);
struct devclass *devclass_by_fd(int fd);
int devclass_fd_to_id(int fd);

void devices_init(void);

#endif /* DEVICE_H */
