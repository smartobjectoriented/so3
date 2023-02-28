/*
 * (C) Copyright 2007
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 *
 * Copyright 2010-2011 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

#include <libfdt/fdt_support.h>
#include <libfdt/libfdt.h>

#define printf( ... )

/*
 * Get cells len in bytes
 *     if #NNNN-cells property is 2 then len is 8
 *     otherwise len is 4
 */
static int get_cells_len(void *blob, char *nr_cells_name)
{
	const fdt32_t *cell;

	cell = fdt_getprop(blob, 0, nr_cells_name, NULL);
	if (cell && fdt32_to_cpu(*cell) == 2)
		return 8;

	return 4;
}

/*
 * Write a 4 or 8 byte big endian cell
 */
static void write_cell(u8 *addr, u64 val, int size)
{
	int shift = (size - 1) * 8;
	while (size-- > 0) {
		*addr++ = (val >> shift) & 0xff;
		shift -= 8;
	}
}

/**
 * fdt_getprop_u32_default - Find a node and return it's property or a default
 *
 * @fdt: ptr to device tree
 * @path: path of node
 * @prop: property name
 * @dflt: default value if the property isn't found
 *
 * Convenience function to find a node and return it's property or a
 * default value if it doesn't exist.
 */
u32 fdt_getprop_u32_default(const void *fdt, const char *path,
				const char *prop, const u32 dflt)
{
	const fdt32_t *val;
	int off;

	off = fdt_path_offset(fdt, path);
	if (off < 0)
		return dflt;

	val = fdt_getprop(fdt, off, prop, NULL);
	if (val)
		return fdt32_to_cpu(*val);
	else
		return dflt;
}

/**
 * fdt_find_and_setprop: Find a node and set it's property
 *
 * @fdt: ptr to device tree
 * @node: path of node
 * @prop: property name
 * @val: ptr to new value
 * @len: length of new property value
 * @create: flag to create the property if it doesn't exist
 *
 * Convenience function to directly set a property given the path to the node.
 */
int fdt_find_and_setprop(void *fdt, const char *node, const char *prop,
			 const void *val, int len, int create)
{
	int nodeoff = fdt_path_offset(fdt, node);

	if (nodeoff < 0)
		return nodeoff;

	if ((!create) && (fdt_get_property(fdt, nodeoff, prop, NULL) == NULL))
		return 0; /* create flag not set; so exit quietly */

	return fdt_setprop(fdt, nodeoff, prop, val, len);
}

int fdt_initrd(void *fdt, ulong initrd_start, ulong initrd_end, int force)
{
	int   nodeoffset, addr_cell_len;
	int   err, j, total;
	fdt64_t  tmp;
	const char *path;
	uint64_t addr, size;

	/* Find the "chosen" node.  */
	nodeoffset = fdt_path_offset (fdt, "/chosen");

	/* If there is no "chosen" node in the blob return */
	if (nodeoffset < 0) {
		printf("fdt_initrd: %s\n", fdt_strerror(nodeoffset));
		return nodeoffset;
	}

	/* just return if initrd_start/end aren't valid */
	if ((initrd_start == 0) || (initrd_end == 0))
		return 0;

	total = fdt_num_mem_rsv(fdt);

	/*
	 * Look for an existing entry and update it.  If we don't find
	 * the entry, we will j be the next available slot.
	 */
	for (j = 0; j < total; j++) {
		err = fdt_get_mem_rsv(fdt, j, &addr, &size);
		if (addr == initrd_start) {
			fdt_del_mem_rsv(fdt, j);
			break;
		}
	}

	err = fdt_add_mem_rsv(fdt, initrd_start, initrd_end - initrd_start);
	if (err < 0) {
		printf("fdt_initrd: %s\n", fdt_strerror(err));
		return err;
	}

	addr_cell_len = get_cells_len(fdt, "#address-cells");

	path = fdt_getprop(fdt, nodeoffset, "linux,initrd-start", NULL);
	if ((path == NULL) || force) {
		write_cell((u8 *)&tmp, initrd_start, addr_cell_len);
		err = fdt_setprop(fdt, nodeoffset,
			"linux,initrd-start", &tmp, addr_cell_len);
		if (err < 0) {
			printf("WARNING: "
				"could not set linux,initrd-start %s.\n",
				fdt_strerror(err));
			return err;
		}
		write_cell((u8 *)&tmp, initrd_end, addr_cell_len);
		err = fdt_setprop(fdt, nodeoffset,
			"linux,initrd-end", &tmp, addr_cell_len);
		if (err < 0) {
			printf("WARNING: could not set linux,initrd-end %s.\n",
				fdt_strerror(err));
			return err;
		}
	}

	return 0;
}

/* Resize the fdt to its actual size + a bit of padding */
int fdt_resize(void *blob)
{
	int i;
	uint64_t addr, size;
	int total, ret;
	uint actualsize;

	if (!blob)
		return 0;

	total = fdt_num_mem_rsv(blob);
	for (i = 0; i < total; i++) {
		fdt_get_mem_rsv(blob, i, &addr, &size);
		if (addr == (uintptr_t)blob) {
			fdt_del_mem_rsv(blob, i);
			break;
		}
	}

	/*
	 * Calculate the actual size of the fdt
	 * plus the size needed for 5 fdt_add_mem_rsv, one
	 * for the fdt itself and 4 for a possible initrd
	 * ((initrd-start + initrd-end) * 2 (name & value))
	 */
	actualsize = fdt_off_dt_strings(blob) +
		fdt_size_dt_strings(blob) + 5 * sizeof(struct fdt_reserve_entry);

	/* Make it so the fdt ends on a page boundary */
	actualsize = ALIGN_UP(actualsize + ((uintptr_t)blob & 0xfff), 0x1000);
	actualsize = actualsize - ((uintptr_t)blob & 0xfff);

	/* Change the fdt header to reflect the correct size */
	fdt_set_totalsize(blob, actualsize);

	/* Add the new reservation */
	ret = fdt_add_mem_rsv(blob, (uintptr_t)blob, actualsize);
	if (ret < 0)
		return ret;

	return actualsize;
}

