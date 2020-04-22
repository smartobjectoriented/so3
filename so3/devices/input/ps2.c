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

/*
 * Helper to deal with the PS/2 communication protocol.
 *
 * Documentation:
 *   https://wiki.osdev.org/Mouse_Input
 *   https://wiki.osdev.org/PS/2_Mouse
 */

#if 0
#define DEBUG
#endif

#include <common.h>
#include <device/input/ps2.h>

#define GET_DX(state, x) ((x) - (((state) << 4) & 0x100))
#define GET_DY(state, y) ((y) - (((state) << 3) & 0x100))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))


/* Extract mouse coordinates and button states from the given packet. */
void get_ps2_state(uint8_t *packet, struct mouse_state * state, uint16_t max_x, uint16_t max_y)
{
	if ((packet[PS2_STATE] & AO)
		/* discard packet if there are overflows, as advised. */
		&& ~(packet[PS2_STATE] & Y_OF)
		&& ~(packet[PS2_STATE] & X_OF)) {

		/*
		 * Do not unset the button value, this is done in the ioctl.
		 * This way the driver has time to read the button state values.
		 */
		if (!state->left) {
			state->left = packet[PS2_STATE] & BL;
		}

		if (!state->right) {
			state->right = packet[PS2_STATE] & BR;
		}

		if (!state->middle) {
			state->middle = packet[PS2_STATE] & BM;
		}

		/* Retrieve dx and dy values to compute the mouse coordinates. */
		state->x += GET_DX(packet[PS2_STATE], packet[PS2_X]);
		state->y -= GET_DY(packet[PS2_STATE], packet[PS2_Y]);

		state->x = CLAMP(state->x, 0, max_x);
		state->y = CLAMP(state->y, 0, max_y);

		DBG("sign_dxy[%s, %s], dxy_val[%03u, %03u], computed_dxy[%03d, %03d]; xy[%03d, %03d]; %03s %03s %03s\n",
			packet[PS2_STATE] & X_BS ? "neg" : "pos",
			packet[PS2_STATE] & Y_BS ? "neg" : "pos",
			packet[PS2_X],
			packet[PS2_Y],
			GET_DX(packet[PS2_STATE], packet[PS2_X]),
			GET_DY(packet[PS2_STATE], packet[PS2_Y]),
			state->x, state->y,
			state->left ? "LFT" : "", state->middle ? "MID" : "", state->right ? "RGT" : "");
	}
	else {
		printk("%s: packet discarded.\n", __func__);
	}
}
