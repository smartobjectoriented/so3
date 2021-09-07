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
#include <common.h>
#include <memory.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>
#include <device/fdt.h>

#include <device/arch/gic.h>

#include <libfdt/libfdt_env.h>

/* Now supported only one level of sub node */
struct fdt_status {
	dev_t *fdt_parent[MAX_SUBNODE];
	int fdt_cur_lvl;
};

static struct fdt_status fdt_cur_status =  {
	.fdt_parent = { NULL },
	.fdt_cur_lvl = 0,
};


/* Initialize dev_info structure to default/invalid values */
static void init_dev_info(dev_t *dev) {

	/* Clear out whole structure */

	memset(dev, 0, sizeof(dev));

	/* Initialize specific fields to default/invalid values */
	dev->base = 0xFFFFFFFF;

	dev->irq_nr = -1;
	dev->irq_type = IRQ_TYPE_NONE;

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


int fdt_get_int(void *fdt_addr, void *dev, const char *name) {
	const struct fdt_property *prop;
	int prop_len;
	fdt32_t *p;
	dev_t *__dev = (dev_t *) dev;

	prop = fdt_get_property((void *) fdt_addr, __dev->offset_dts, name, &prop_len);

	if (prop) {
		p = (fdt32_t *) prop->data;

		return fdt32_to_cpu(p[0]);
	} else
		return -1;
}

const struct fdt_property *fdt_find_property(void *fdt_addr, int offset, const char *propname) {
	const struct fdt_property *prop;

	prop = fdt_get_property(fdt_addr, offset, propname, NULL);
	if (prop)
		return prop;
	else
		return NULL;
}

int fdt_property_read_string(void *fdt_addr, int offset, const char *propname, const char **out_string) {
	const struct fdt_property *prop;

	prop = fdt_find_property(fdt_addr, offset, propname);
	if (prop) {
		*out_string = prop->data;
		return 0;
	}

	return -1;
}

int fdt_property_read_u32(void *fdt_addr, int offset, const char *propname, u32 *out_value) {
	const fdt32_t *val;

	val = fdt_getprop(fdt_addr, offset, propname, NULL);

	if (val) {
		*out_value = fdt32_to_cpu(val[0]);
		return 0;
	}

	return -1;
}

int fdt_property_read_u64(void *fdt_addr, int offset, const char *propname, u64 *out_value) {
	const fdt64_t *val;

	val = fdt_getprop(fdt_addr, offset, propname, NULL);

	if (val) {
		*out_value = fdt64_to_cpu(val[0]);
		return 0;
	}

	return -1;
}

int fdt_find_node_by_name(void *fdt_addr, int parent, const char *nodename) {
	int node;
	const char *__nodename, *node_name;
	int len;

	fdt_for_each_subnode(node, fdt_addr, parent) {
		__nodename = fdt_get_name(fdt_addr, node, &len);

		node_name = kbasename(__nodename);
		len = strchrnul(node_name, '@') - node_name;

		if ((strlen(nodename) == len) && (strncmp(node_name, nodename, len) == 0))
			return node;

	}

	return -1;
}

/*
 * Retrieve a node matching with a specific compat string.
 * Returns -1 if no node is present.
 */
int fdt_find_compatible_node(void *fdt_addr, char *compat) {
	int offset;

	offset = fdt_node_offset_by_compatible(fdt_addr, 0, compat);

	return offset;
}
/*
 * fdt_pack_reg - pack address and size array into the "reg"-suitable stream
 */
int fdt_pack_reg(const void *fdt, void *buf, u64 *address, u64 *size)
{
	int address_cells = fdt_address_cells(fdt, 0);
	int size_cells = fdt_size_cells(fdt, 0);
	char *p = buf;

	if (address_cells == 2)
		*(fdt64_t *)p = cpu_to_fdt64(*address);
	else
		*(fdt32_t *)p = cpu_to_fdt32(*address);

	p += 4 * address_cells;

	if (size_cells == 2)
		*(fdt64_t *)p = cpu_to_fdt64(*size);
	else
		*(fdt32_t *)p = cpu_to_fdt32(*size);

	p += 4 * size_cells;

	return p - (char *)buf;
}

/**
 * fdt_find_or_add_subnode() - find or possibly add a subnode of a given node
 *
 * @fdt: pointer to the device tree blob
 * @parentoffset: structure block offset of a node
 * @name: name of the subnode to locate
 *
 * fdt_subnode_offset() finds a subnode of the node with a given name.
 * If the subnode does not exist, it will be created.
 */
int fdt_find_or_add_subnode(void *fdt, int parentoffset, const char *name)
{
	int offset;

	offset = fdt_subnode_offset(fdt, parentoffset, name);

	if (offset == -FDT_ERR_NOTFOUND)
		offset = fdt_add_subnode(fdt, parentoffset, name);

	if (offset < 0)
		printk("%s: %s: %s\n", __func__, name, fdt_strerror(offset));

	return offset;
}

/* Get device informations/parameters from a device tree */
int get_dev_info(const void *fdt_addr, int offset, const char *compat, void *info) {
	int new_offset;
	const struct fdt_property *prop;
	int prop_len;
	const fdt32_t *p;
	const char *compat_str, *node_str;
	static int depth = 0;
	uint32_t irq_gic_type;
	dev_t *__info = (dev_t *) info;

	/* Need to reset the depth? */
	if (offset == 0)
		depth = 0;

	/* Initialize dev_info structure to default/invalid values */
	init_dev_info(info);

	/* Find first compatible node in tree */
	if (strcmp(compat, "*") == 0)
		new_offset = fdt_next_node(fdt_addr, offset, &depth);
	else
		new_offset = fdt_node_offset_by_compatible(fdt_addr, offset, compat);


	if (new_offset < 0)
		/* No node found */
		return -1;

	if (depth > MAX_SUBNODE) {
		printk("Cannot enter subnode, 4 subnode max \n");
		BUG();

	}

	fdt_cur_status.fdt_parent[fdt_cur_status.fdt_cur_lvl] = info;

	if (!depth) {
		__info->parent = NULL;
	}
	else if (depth > fdt_cur_status.fdt_cur_lvl) {
		fdt_cur_status.fdt_cur_lvl++;
	}
	else if (depth < fdt_cur_status.fdt_cur_lvl) {
		fdt_cur_status.fdt_cur_lvl--;
	}
	else {
		__info->parent = fdt_cur_status.fdt_parent[fdt_cur_status.fdt_cur_lvl-1];
	}

	__info->offset_dts = new_offset;
	__info->fdt = (void *) fdt_addr;

	compat_str = fdt_getprop(fdt_addr, new_offset, "compatible", &prop_len);

	if (!compat_str) {
		return new_offset;
	}

	if (prop_len > MAX_COMPAT_SIZE) {
		DBG("Length of Compatible string > %d chars\n", MAX_COMPAT_SIZE);
		return new_offset;
	}

	strncpy(__info->compatible, compat_str, prop_len);

	node_str = fdt_get_name(fdt_addr, new_offset, &prop_len);
	if (prop_len > MAX_COMPAT_SIZE) {
	 	DBG("Length of Compatible string > %d chars\n", MAX_NODE_SIZE);
		return new_offset;
	}

	strncpy(__info->nodename, node_str, prop_len);

	prop = fdt_get_property(fdt_addr, new_offset, "status", &prop_len);

	if (prop) {
		if (!strcmp(prop->data, "disabled"))
			__info->status = STATUS_DISABLED;
		else if (!strcmp(prop->data, "ok"))
			__info->status = STATUS_INIT_PENDING;
	}

	prop = fdt_get_property(fdt_addr, new_offset, "reg", &prop_len);
	
	if (prop) {
		
		p = (const fdt32_t *) prop->data;
				
		__info->base = fdt32_to_cpu(p[0]);

		if (prop_len > sizeof(uint32_t)) {
			/* We have a size information */
			__info->size = fdt32_to_cpu(p[1]);
		} else {
			__info->size = 0;
		}
		__info->irq_type = fdt32_to_cpu(p[2]);
	} 
	
	/* Interrupts - as described in the bindings - have 3 specific cells */
	prop = fdt_get_property(fdt_addr, new_offset, "interrupts", &prop_len);

	if (prop) {		
		p = (const fdt32_t *) prop->data;

		if (prop_len == 3 * sizeof(uint32_t)) {

			/* Retrieve the 3-cell values */
			irq_gic_type = fdt32_to_cpu(p[0]);
			__info->irq_nr = fdt32_to_cpu(p[1]);
			__info->irq_type = fdt32_to_cpu(p[3]);

			/* Not all combinations are currently handled. */

			if (irq_gic_type != GIC_IRQ_TYPE_SGI)
				__info->irq_nr += 16; /* Possibly for a Private Peripheral Interrupt (PPI) */

			if (irq_gic_type == GIC_IRQ_TYPE_SPI) /* It is a Shared Peripheral Interrupt (SPI) */
				__info->irq_nr += 16;

		} else {
			/* Unsupported size of interrupts property */
			lprintk("%s: unsupported size of interrupts property\n");
			BUG();
		}
	}

	/* We got all required information, the device is ready to be initialized */
	__info->status = STATUS_INIT_PENDING;

	return new_offset;
}

