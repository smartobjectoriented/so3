/*
 * Copyright (C) 2016-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2016-2018 Baptiste Delporte <bonel@bonel.net>
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

#if 0
#define DEBUG
#endif

#include <device/fdt.h>

#include <soo/console.h>
#include <soo/debug.h>
#include <soo/vbus.h>

/* capsule ID related information management */

/**
 * Get the short description related to this capsule.
 * (not mandatory)
 *
 * @return a pointer to the string in the DT if it exists, NULL otherwise.
 */
const char *get_s3c_shortdesc(void)
{
	const char *str = NULL;
	int node;

	/* Get the short description */
	node = fdt_find_node_by_name(__fdt_addr, 0, "capsule");
	ASSERT(node >= 0);

	fdt_property_read_string(__fdt_addr, node, "s3c_shortdesc", &str);

	return str;
}

/**
 * Get the name of this capsule.
 * (not mandatory)
 *
 * @return a pointer to the string in the DT if it exists, NULL otherwise.
 */
const char *get_s3c_name(void)
{
	const char *str = NULL;
	int node;

	/* Get the short description */
	node = fdt_find_node_by_name(__fdt_addr, 0, "capsule");
	ASSERT(node >= 0);

	fdt_property_read_string(__fdt_addr, node, "s3c_name", &str);

	return str;
}

/**
 * Get the SPID related to this capsule.
 * (mandatory)
 *
 * @param what  Either "spid"
 * @return SPID on 64-bit encoding
 */
u64 get_spid(void)
{
	u64 val;
	int node;

	/* Get the short description */
	node = fdt_find_node_by_name(__fdt_addr, 0, "capsule");
	if (node < 0) {
		printk("%s: node \"capsule\" not found\n", __func__);
		BUG();
	}

	node = fdt_property_read_u64(__fdt_addr, node, "spid", &val);
	if (node < 0) {
		printk("%s: node \"%s\" not found\n", __func__, "spid");
		BUG();
	}

	return val;
}

/**
 * Write the entries related to the capsule ID in vbstore
 */
void vbstore_S3C_ID_populate(void)
{
	const char *name, *shortdesc;
	u64 spid;
	char rootname[VBS_KEY_LENGTH], entry[VBS_KEY_LENGTH];

	/* Set all capsule ID related information */

	/* Set the SPID of this capsule */
	spid = get_spid();

	avz_shared->dom_desc.u.S3C.spid = spid;

	/* Set the name */
	name = get_s3c_name();

	/* And set a short description which can be used on the user GUI */
	shortdesc = get_s3c_shortdesc();

	strcpy(rootname, "soo/s3c");

	sprintf(entry, "%d", S3C_domID());
	vbus_mkdir(VBT_NIL, rootname, entry);

	sprintf(rootname, "soo/s3c/%d", S3C_domID());
	sprintf(entry, "%lx", spid);

	vbus_write(VBT_NIL, rootname, "spid", entry);

	vbus_write(VBT_NIL, rootname, "name", name);
	vbus_write(VBT_NIL, rootname, "shortdesc", shortdesc);
}
