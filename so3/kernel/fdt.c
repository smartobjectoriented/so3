/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 *
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

#include <string.h>
#include <linker.h>
#include <common.h>
#include <memory.h>

#include <device/device.h>
#include <device/driver.h>

#include <device/fdt/fdt.h>
#include <device/fdt/libfdt.h>

/* Now supported only one level of sub node */
struct fdt_status {
	dev_t *fdt_parent[MAX_SUBNODE];
	int fdt_cur_lvl;
};

static struct fdt_status fdt_cur_status =  {
			  .fdt_parent = {NULL},
			  .fdt_cur_lvl = 0,
};


/* Initialize dev_info structure to default/invalid values */
static void init_dev_info(dev_t *dev) {

	/* Clear out whole structure */

	memset(dev, 0, sizeof(dev));

	/* Initialize specific fields to default/invalid values */
	dev->base = 0xFFFFFFFF;
	dev->irq = -1;
	dev->status = STATUS_UNKNOWN;
	dev->offset_dts = -1;
	dev->parent = NULL;
	dev->fdt = 0;
}

/* Get memory informations from a device tree */
int get_mem_info(const void *fdt, mem_info_t *info) {
	int offset;
	const struct fdt_property *prop;
	const fdt32_t *p;

	offset = fdt_path_offset(fdt, "/memory");

	/* "memory" node not found --> error */
	if (offset < 0) {
		return offset;
	}

	prop = fdt_get_property(fdt, offset, "reg", NULL);

	if (prop) {
		
		p = (const fdt32_t *)prop->data;
				
		info->phys_base = fdt32_to_cpu(p[0]);
		info->size = fdt32_to_cpu(p[1]);
	}

	return offset;
}


int fdt_get_int(dev_t *dev, const char *name) {
	const struct fdt_property *prop;
	int prop_len;
	fdt32_t *p;

	prop = fdt_get_property((void *) _fdt_addr, dev->offset_dts, name, &prop_len);

	if (prop) {
		p = (fdt32_t *) prop->data;

		return fdt32_to_cpu(p[0]);
	} else
		return -1;
}

const struct fdt_property *find_prop(const char *propname) {
	const struct fdt_property *prop;
	int offset;

	while (true) {
		offset = fdt_next_node((const void *) _fdt_addr, offset, NULL);
		if (offset < 0)
			return NULL;
		prop = fdt_get_property((const void *) _fdt_addr, offset, "realtime", NULL);
		if (prop)
			return prop;
	}

	return NULL;
}

/*
 * Retrieve a node matching with a specific compat string.
 * Returns -1 if no node is present.
 */
int fdt_find_compatible_node(char *compat) {
	int offset;

	offset = fdt_node_offset_by_compatible((void *) _fdt_addr, 0, compat);

	return offset;
}

/* Get device informations/parameters from a device tree */
int get_dev_info(const void *fdt, int offset, const char *compat, dev_t *info) {
	int new_offset;
	const struct fdt_property *prop;
	int prop_len;
	const fdt32_t *p;
	const char *compat_str, *node_str;
	static int depth = 0;

	/* Need to reset the depth? */
	if (offset == 0)
		depth = 0;

	/* Initialize dev_info structure to default/invalid values */
	init_dev_info(info);

	/* Find first compatible node in tree */
	if (strcmp(compat, "*") == 0)
		new_offset = fdt_next_node(fdt, offset, &depth);
	else
		new_offset = fdt_node_offset_by_compatible(fdt, offset, compat);


	if (new_offset < 0)
		/* No node found */
		return -1;

	if (depth > MAX_SUBNODE) {
		printk("Cannot enter subnode, 4 subnode max \n");
		BUG();

	}

	fdt_cur_status.fdt_parent[fdt_cur_status.fdt_cur_lvl] = info;

	if (!depth) {
		info->parent = NULL;
	}
	else if (depth > fdt_cur_status.fdt_cur_lvl) {
		fdt_cur_status.fdt_cur_lvl++;
	}
	else if (depth < fdt_cur_status.fdt_cur_lvl) {
		fdt_cur_status.fdt_cur_lvl--;
	}
	else {
		info->parent = fdt_cur_status.fdt_parent[fdt_cur_status.fdt_cur_lvl-1];
	}

	info->offset_dts = new_offset;
	info->fdt = (void *) fdt;

	compat_str = fdt_getprop(fdt, new_offset, "compatible", &prop_len);

	if (!compat_str) {
		return new_offset;
	}

	if (prop_len > MAX_COMPAT_SIZE) {
		DBG("Length of Compatible string > %d chars\n", MAX_COMPAT_SIZE);
		return new_offset;
	}

	strncpy(info->compatible, compat_str, prop_len);

	node_str = fdt_get_name(fdt, new_offset, &prop_len);
	if (prop_len > MAX_COMPAT_SIZE) {
	 	DBG("Length of Compatible string > %d chars\n", MAX_NODE_SIZE);
		return new_offset;
	}

	strncpy(info->nodename, node_str, prop_len);

	prop = fdt_get_property(fdt, new_offset, "status", &prop_len);

	if (prop) {
		if (!strcmp(prop->data, "disabled"))
			info->status = STATUS_DISABLED;
		else if (!strcmp(prop->data, "ok"))
			info->status = STATUS_INIT_PENDING;
	}

	prop = fdt_get_property(fdt, new_offset, "reg", &prop_len);
	
	if (prop) {
		
		p = (const fdt32_t *) prop->data;
				
			info->base = fdt32_to_cpu(p[0]);

			if (prop_len > sizeof(uint32_t)) {
				/* We have a size information */
				info->size = fdt32_to_cpu(p[1]);
			} else {
				info->size = 0;
			}
		
	} 
	
	prop = fdt_get_property(fdt, new_offset, "interrupts", &prop_len);

	if (prop) {		
		p = (const fdt32_t *) prop->data;
		
		if (prop_len == sizeof(uint32_t))
			info->irq = fdt32_to_cpu(p[0]);
		else
			/* Unsupported size of interrupts property */
			return -1;
	}

	/* We got all required information, the device is ready to be initialized */
	info->status = STATUS_INIT_PENDING;

	return new_offset;
}

