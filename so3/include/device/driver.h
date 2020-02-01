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

#include <linker.h>

#include <device/fdt/fdt.h>
#include <device/device.h>

/* driver registering */

struct driver_entry {
	char *compatible; /* compatible Name */

	int (*init)(dev_t *dev);
};
typedef struct driver_entry driver_entry_t;

#define REGISTER_DRIVER_GENERIC(_compatible, _init, _level) ll_entry_declare(driver_entry_t, _level, _init) = { \
		.compatible = _compatible,	\
		.init = _init, \
}

#define REGISTER_DRIVER_CORE(_compatible, _init) REGISTER_DRIVER_GENERIC(_compatible, _init, core)
#define REGISTER_DRIVER_POSTCORE(_compatible, _init) REGISTER_DRIVER_GENERIC(_compatible, _init, postcore)


void init_driver_from_dtb(void);

#endif /* DRIVER_H */
