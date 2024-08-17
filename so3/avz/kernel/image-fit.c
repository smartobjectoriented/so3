// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2013, Google Inc.
 *
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <types.h>

#include <string.h>
#include <console.h>
#include <errno.h>

#include <asm/byteorder.h>

#include <libfdt/fdt_support.h>
#include <libfdt/image.h>

#define uimage_to_cpu(x)		be32_to_cpu(x)

/*****************************************************************************/
/* New uImage format routines */
/*****************************************************************************/

static int fit_parse_spec(const char *spec, char sepc, ulong addr_curr,
		ulong *addr, const char **name)
{
	const char *sep;

	*addr = addr_curr;
	*name = NULL;

	sep = strchr(spec, sepc);
	if (sep) {
		if (sep - spec > 0)
			*addr = simple_strtoul(spec, NULL, 16);

		*name = sep + 1;
		return 1;
	}

	return 0;
}

/**
 * fit_parse_conf - parse FIT configuration spec
 * @spec: input string, containing configuration spec
 * @add_curr: current image address (to be used as a possible default)
 * @addr: pointer to a ulong variable, will hold FIT image address of a given
 * configuration
 * @conf_name double pointer to a char, will hold pointer to a configuration
 * unit name
 *
 * fit_parse_conf() expects configuration spec in the form of [<addr>]#<conf>,
 * where <addr> is a FIT image address that contains configuration
 * with a <conf> unit name.
 *
 * Address part is optional, and if omitted default add_curr will
 * be used instead.
 *
 * returns:
 *     1 if spec is a valid configuration string,
 *     addr and conf_name are set accordingly
 *     0 otherwise
 */
int fit_parse_conf(const char *spec, ulong addr_curr,
		ulong *addr, const char **conf_name)
{
	return fit_parse_spec(spec, '#', addr_curr, addr, conf_name);
}

/**
 * fit_parse_subimage - parse FIT subimage spec
 * @spec: input string, containing subimage spec
 * @add_curr: current image address (to be used as a possible default)
 * @addr: pointer to a ulong variable, will hold FIT image address of a given
 * subimage
 * @image_name: double pointer to a char, will hold pointer to a subimage name
 *
 * fit_parse_subimage() expects subimage spec in the form of
 * [<addr>]:<subimage>, where <addr> is a FIT image address that contains
 * subimage with a <subimg> unit name.
 *
 * Address part is optional, and if omitted default add_curr will
 * be used instead.
 *
 * returns:
 *     1 if spec is a valid subimage string,
 *     addr and image_name are set accordingly
 *     0 otherwise
 */
int fit_parse_subimage(const char *spec, ulong addr_curr,
		ulong *addr, const char **image_name)
{
	return fit_parse_spec(spec, ':', addr_curr, addr, image_name);
}



static void fit_get_debug(const void *fit, int noffset,
		char *prop_name, int err)
{
	printk("Can't get '%s' property from FIT 0x%08lx, node: offset %d, name %s (%s)\n",
	      prop_name, (ulong)fit, noffset, fit_get_name(fit, noffset, NULL),
	      fdt_strerror(err));
}


/**
 * fit_image_get_node - get node offset for component image of a given unit name
 * @fit: pointer to the FIT format image header
 * @image_uname: component image node unit name
 *
 * fit_image_get_node() finds a component image (within the '/images'
 * node) of a provided unit name. If image is found its node offset is
 * returned to the caller.
 *
 * returns:
 *     image node offset when found (>=0)
 *     negative number on failure (FDT_ERR_* code)
 */
int fit_image_get_node(const void *fit, const char *image_uname)
{
	int noffset, images_noffset;

	images_noffset = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images_noffset < 0) {
		printk("Can't find images parent node '%s' (%s)\n", FIT_IMAGES_PATH, fdt_strerror(images_noffset));
		return images_noffset;
	}

	noffset = fdt_subnode_offset(fit, images_noffset, image_uname);
	if (noffset < 0) {
		printk("Can't get node offset for image unit name: '%s' (%s)\n",
		      image_uname, fdt_strerror(noffset));
	}

	return noffset;
}

/**
 * fit_image_get_os - get os id for a given component image node
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @os: pointer to the uint8_t, will hold os numeric id
 *
 * fit_image_get_os() finds os property in a given component image node.
 * If the property is found, its (string) value is translated to the numeric
 * id which is returned to the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_get_os(const void *fit, int noffset, uint8_t *os)
{
	int len;
	const void *data;

	/* Get OS name from property data */
	data = fdt_getprop(fit, noffset, FIT_OS_PROP, &len);
	if (data == NULL) {
		fit_get_debug(fit, noffset, FIT_OS_PROP, len);
		*os = -1;
		return -1;
	}

	/* Translate OS name to id */
#if 0
	*os = genimg_get_os_id(data);
#endif
	return 0;
}

/**
 * fit_image_get_arch - get arch id for a given component image node
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @arch: pointer to the uint8_t, will hold arch numeric id
 *
 * fit_image_get_arch() finds arch property in a given component image node.
 * If the property is found, its (string) value is translated to the numeric
 * id which is returned to the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_get_arch(const void *fit, int noffset, uint8_t *arch)
{
	int len;
	const void *data;

	/* Get architecture name from property data */
	data = fdt_getprop(fit, noffset, FIT_ARCH_PROP, &len);
	if (data == NULL) {
		fit_get_debug(fit, noffset, FIT_ARCH_PROP, len);
		*arch = -1;
		return -1;
	}

	/* Translate architecture name to id */
#if 0
	*arch = genimg_get_arch_id(data);
#endif
	return 0;
}

/**
 * fit_image_get_type - get type id for a given component image node
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @type: pointer to the uint8_t, will hold type numeric id
 *
 * fit_image_get_type() finds type property in a given component image node.
 * If the property is found, its (string) value is translated to the numeric
 * id which is returned to the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_get_type(const void *fit, int noffset, uint8_t *type)
{
	int len;
	const void *data;

	/* Get image type name from property data */
	data = fdt_getprop(fit, noffset, FIT_TYPE_PROP, &len);
	if (data == NULL) {
		fit_get_debug(fit, noffset, FIT_TYPE_PROP, len);
		*type = -1;
		return -1;
	}

	/* Translate image type name to id */
#if 0
	*type = genimg_get_type_id(data);
#endif
	return 0;
}

/**
 * fit_image_get_comp - get comp id for a given component image node
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @comp: pointer to the uint8_t, will hold comp numeric id
 *
 * fit_image_get_comp() finds comp property in a given component image node.
 * If the property is found, its (string) value is translated to the numeric
 * id which is returned to the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_get_comp(const void *fit, int noffset, uint8_t *comp)
{
	int len;
	const void *data;

	/* Get compression name from property data */
	data = fdt_getprop(fit, noffset, FIT_COMP_PROP, &len);
	if (data == NULL) {
		fit_get_debug(fit, noffset, FIT_COMP_PROP, len);
		*comp = -1;
		return -1;
	}

	/* Translate compression name to id */
#if 0
	*comp = genimg_get_comp_id(data);
#endif
	return 0;
}

static int fit_image_get_address(const void *fit, int noffset, char *name,
			  ulong *load)
{
	int len, cell_len;
	const fdt32_t *cell;
	uint64_t load64 = 0;

	cell = fdt_getprop(fit, noffset, name, &len);
	if (cell == NULL) {
		fit_get_debug(fit, noffset, name, len);
		return -1;
	}

	if (len > sizeof(ulong)) {
		printk("Unsupported %s address size\n", name);
		return -1;
	}

	cell_len = len >> 2;
	/* Use load64 to avoid compiling warning for 32-bit target */
	while (cell_len--) {
		load64 = (load64 << 32) | uimage_to_cpu(*cell);
		cell++;
	}
	*load = (ulong)load64;

	return 0;
}
/**
 * fit_image_get_load() - get load addr property for given component image node
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @load: pointer to the uint32_t, will hold load address
 *
 * fit_image_get_load() finds load address property in a given component
 * image node. If the property is found, its value is returned to the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_get_load(const void *fit, int noffset, ulong *load)
{
	return fit_image_get_address(fit, noffset, FIT_LOAD_PROP, load);
}

/**
 * fit_image_get_entry() - get entry point address property
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @entry: pointer to the uint32_t, will hold entry point address
 *
 * This gets the entry point address property for a given component image
 * node.
 *
 * fit_image_get_entry() finds entry point address property in a given
 * component image node.  If the property is found, its value is returned
 * to the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_get_entry(const void *fit, int noffset, ulong *entry)
{
	return fit_image_get_address(fit, noffset, FIT_ENTRY_PROP, entry);
}

/**
 * fit_image_get_data - get data property and its size for a given component image node
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @data: double pointer to void, will hold data property's data address
 * @size: pointer to size_t, will hold data property's data size
 *
 * fit_image_get_data() finds data property in a given component image node.
 * If the property is found its data start address and size are returned to
 * the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_get_data(const void *fit, int noffset,
		const void **data, size_t *size)
{
	int len;

	*data = fdt_getprop(fit, noffset, FIT_DATA_PROP, &len);
	if (*data == NULL) {
		fit_get_debug(fit, noffset, FIT_DATA_PROP, len);
		*size = 0;
		return -1;
	}

	*size = len;
	return 0;
}

/**
 * Get 'data-offset' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_offset: holds the data-offset property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */
int fit_image_get_data_offset(const void *fit, int noffset, int *data_offset)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_OFFSET_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_offset = fdt32_to_cpu(*val);

	return 0;
}

/**
 * Get 'data-position' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_position: holds the data-position property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */
int fit_image_get_data_position(const void *fit, int noffset,
				int *data_position)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_POSITION_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_position = fdt32_to_cpu(*val);

	return 0;
}

/**
 * Get 'data-size' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_size: holds the data-size property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */
int fit_image_get_data_size(const void *fit, int noffset, int *data_size)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_SIZE_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_size = fdt32_to_cpu(*val);

	return 0;
}

/**
 * fit_image_get_data_and_size - get data and its size including
 *				 both embedded and external data
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @data: double pointer to void, will hold data property's data address
 * @size: pointer to size_t, will hold data property's data size
 *
 * fit_image_get_data_and_size() finds data and its size including
 * both embedded and external data. If the property is found
 * its data start address and size are returned to the caller.
 *
 * returns:
 *     0, on success
 *     otherwise, on failure
 */
int fit_image_get_data_and_size(const void *fit, int noffset,
				const void **data, size_t *size)
{
	bool external_data = false;
	int offset;
	int len;
	int ret;

	if (!fit_image_get_data_position(fit, noffset, &offset)) {
		external_data = true;
	} else if (!fit_image_get_data_offset(fit, noffset, &offset)) {
		external_data = true;
		/*
		 * For FIT with external data, figure out where
		 * the external images start. This is the base
		 * for the data-offset properties in each image.
		 */
		offset += ((fdt_totalsize(fit) + 3) & ~3);
	}

	if (external_data) {
		printk("External Data\n");
		ret = fit_image_get_data_size(fit, noffset, &len);
		*data = fit + offset;
		*size = len;
	} else {
		ret = fit_image_get_data(fit, noffset, data, size);
	}

	return ret;
}

/**
 * fit_image_hash_get_algo - get hash algorithm name
 * @fit: pointer to the FIT format image header
 * @noffset: hash node offset
 * @algo: double pointer to char, will hold pointer to the algorithm name
 *
 * fit_image_hash_get_algo() finds hash algorithm property in a given hash node.
 * If the property is found its data start address is returned to the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_hash_get_algo(const void *fit, int noffset, char **algo)
{
	int len;

	*algo = (char *)fdt_getprop(fit, noffset, FIT_ALGO_PROP, &len);
	if (*algo == NULL) {
		fit_get_debug(fit, noffset, FIT_ALGO_PROP, len);
		return -1;
	}

	return 0;
}

/**
 * fit_image_hash_get_value - get hash value and length
 * @fit: pointer to the FIT format image header
 * @noffset: hash node offset
 * @value: double pointer to uint8_t, will hold address of a hash value data
 * @value_len: pointer to an int, will hold hash data length
 *
 * fit_image_hash_get_value() finds hash value property in a given hash node.
 * If the property is found its data start address and size are returned to
 * the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_hash_get_value(const void *fit, int noffset, uint8_t **value,
				int *value_len)
{
	int len;

	*value = (uint8_t *)fdt_getprop(fit, noffset, FIT_VALUE_PROP, &len);
	if (*value == NULL) {
		fit_get_debug(fit, noffset, FIT_VALUE_PROP, len);
		*value_len = 0;
		return -1;
	}

	*value_len = len;
	return 0;
}

