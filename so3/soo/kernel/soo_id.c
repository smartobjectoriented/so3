/*
 * Copyright (C) 2016-2021 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <soo/soo.h>
#include <soo/console.h>
#include <soo/debug.h>
#include <soo/vbus.h>

/* ME ID related information management */

/**
 * Get the short description related to this ME.
 * (not mandatory)
 *
 * @return a pointer to the string in the DT if it exists, NULL otherwise.
 */
const char *get_me_shortdesc(void) {
	const char *str = NULL;
	int node;

	/* Get the short description */
	node = fdt_find_node_by_name(__fdt_addr, 0, "ME");
	ASSERT(node >= 0);

	fdt_property_read_string(__fdt_addr, node, "me_shortdesc", &str);

	return str;
}

/**
 * Get the name of this ME.
 * (not mandatory)
 *
 * @return a pointer to the string in the DT if it exists, NULL otherwise.
 */
const char *get_me_name(void) {
	const char *str = NULL;
	int node;

	/* Get the short description */
	node = fdt_find_node_by_name(__fdt_addr, 0, "ME");
	ASSERT(node >= 0);

	fdt_property_read_string(__fdt_addr, node, "me_name", &str);

	return str;
}

/**
 * Get the SPID related to this ME.
 * (mandatory)
 *
 * @param what  Either "spid" or "spad"
 * @return SPID on 64-bit encoding
 */
u64 get_spid_spad(char *what) {
	u64 val;
	int node;

	/* Get the short description */
	node = fdt_find_node_by_name(__fdt_addr, 0, "ME");
	if (node < 0) {
		printk("%s: node \"ME\" not found\n", __func__);
		BUG();
	}

	node = fdt_property_read_u64(__fdt_addr, node, what, &val);
	if (node < 0) {
		printk("%s: node \"%s\" not found\n", __func__, what);
		BUG();
	}

	return val;
}

u64 get_spid(void) {
	return get_spid_spad("spid");
}

/**
 * Write the entries related to the ME ID in vbstore
 */
void vbstore_ME_ID_populate(void) {
	const char *name, *shortdesc;
	u64 spid, spadcaps;
	char rootname[VBS_KEY_LENGTH], entry[VBS_KEY_LENGTH];

	/* Set all ME ID related information */

	/* Set the SPID of this ME */
	spid = get_spid_spad("spid");
	spadcaps = get_spid_spad("spadcaps");

	avz_shared->dom_desc.u.ME.spid = spid;
	avz_shared->dom_desc.u.ME.spad.spadcaps = spadcaps;

	/* Set the name */
	name = get_me_name();

	/* And set a short description which can be used on the user GUI */
	shortdesc = get_me_shortdesc();

	strcpy(rootname, "soo/me");

	sprintf(entry, "%d", ME_domID());
	vbus_mkdir(VBT_NIL, rootname, entry);

	sprintf(rootname, "soo/me/%d", ME_domID());
	sprintf(entry, "%llx", spid);

	vbus_write(VBT_NIL, rootname, "spid", entry);

	sprintf(entry, "%llx", spadcaps);

	vbus_write(VBT_NIL, rootname, "spadcaps", entry);

	vbus_write(VBT_NIL, rootname, "name", name);
	vbus_write(VBT_NIL, rootname, "shortdesc", shortdesc);

}

