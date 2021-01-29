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

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/* Mouse packet byte indexes. */
#define PS2_STATE 0
#define PS2_X     1
#define PS2_Y     2

/* Mouse commands. */
#define EN_PKT_STREAM 0xf4

/* Mouse state packet byte masks. */
#define Y_OF (1 << 7)
#define X_OF (1 << 6)
#define Y_BS (1 << 5)
#define X_BS (1 << 4)
#define AO   (1 << 3)
#define BM   (1 << 2)
#define BR   (1 << 1)
#define BL   (1 << 0)

/* Keyboard bytes */
#define KEY_LSH 0x12
#define KEY_RSH 0x59
#define KEY_EXT 0xe0
#define KEY_REL 0xf0

/* Keyboard masks */
#define KEY_ST_PRESSED  (1 << 0)
#define KEY_ST_SHIFT    (1 << 1)
#define KEY_ST_EXTENDED (1 << 2)


struct ps2_mouse {
	int16_t x, y;
	uint8_t left, right, middle;
};

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

void get_mouse_state(uint8_t *packet, struct ps2_mouse *, uint16_t max_x, uint16_t max_y);
void get_kb_key(uint8_t *packet, uint8_t len, struct ps2_key *);
