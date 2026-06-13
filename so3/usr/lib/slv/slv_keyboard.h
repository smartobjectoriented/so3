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

#ifndef SLV_KEYBOARD_H
#define SLV_KEYBOARD_H

#include <stdint.h>

#define IOCTL_KB_GET_KEY 0

#define KB_DEV		 "/dev/keyboard"

struct ps2_key {
	/* UTF-8 char */
	uint32_t value;

	/*
	 * 1st bit -> whether key was pressed or released
	 * 2nd bit -> whether the shift modifier was activated or not
	 * 3rd bit -> indicates if the first scan codes was 0xE0
	 */
	uint8_t state;
};

int slv_keyboard_init(void);
void slv_keyboard_terminate(int fd);

#endif
