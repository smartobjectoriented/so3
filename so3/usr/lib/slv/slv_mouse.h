/*
 * Copyright (C) 2025 André Costa <andre_miguel_costa@hotmail.com>
 * Copyright (C) 2020 Nikolaos Garanis <nikolaos.garanis@heig-vd.ch>
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

#ifndef SLV_MOUSE_H
#define SLV_MOUSE_H

#include <stdint.h>

#define IOCTL_MOUSE_GET_STATE 0
#define IOCTL_MOUSE_SET_RES   1

#define MOUSE_DEV	      "/dev/mouse"

struct ps2_mouse {
	uint16_t x, y;
	uint8_t left, right, middle;
};

struct display_res {
	uint16_t h, v;
};

int slv_mouse_init(uint16_t hres, uint16_t vres);
void slv_mouse_terminate(int fd);

#endif
