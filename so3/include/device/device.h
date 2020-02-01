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

#include <device/fdt/fdt.h>

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

#define INITCALLS_LEVELS 2

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

void devices_init(void);

#endif /* DEVICE_H */
