/*
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

#include <types.h>

/* PS2 packet byte indexes. */
#define PS2_STATE 0
#define PS2_X     1
#define PS2_Y     2

/* PS/2 commands. */
#define EN_PKT_STREAM 0xf4

/* First packet byte masks. */
#define Y_OF (1 << 7)
#define X_OF (1 << 6)
#define Y_BS (1 << 5)
#define X_BS (1 << 4)
#define AO   (1 << 3)
#define BM   (1 << 2)
#define BR   (1 << 1)
#define BL   (1 << 0)


struct mouse_state {
	uint16_t x, y;
	uint8_t left, right, middle;
};

void get_ps2_state(uint8_t *packet, struct mouse_state *, uint16_t max_x, uint16_t max_y);
