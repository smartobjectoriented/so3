/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2019 David Truan <david.truan@heig-vd.ch>
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

#ifndef DRIVER_H
#define DRIVER_H

#include <initcall.h>

#include <device/device.h>

/* driver registering */

struct driver_initcall {
	char *compatible; /* compatible string */

	int (*init)(dev_t *dev, int fdt_offset);
};
typedef struct driver_initcall driver_initcall_t;

#define REGISTER_DRIVER_INITCALL(_compatible, _level, _init) ll_entry_declare(driver_initcall_t, _level, _init) = { \
		.compatible = _compatible,	\
		.init = _init, \
}

#define REGISTER_DRIVER_CORE(_compatible, _init) REGISTER_DRIVER_INITCALL(_compatible, core, _init)
#define REGISTER_DRIVER_POSTCORE(_compatible, _init) REGISTER_DRIVER_INITCALL(_compatible, postcore, _init)


void init_driver_from_dtb(void);

#endif /* DRIVER_H */
