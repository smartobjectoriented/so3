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
 * Virtual driver for the mouse.
 */

#if 0
#define DEBUG
#endif

#include <vfs.h>
#include <common.h>

#include <asm/io.h>

#include <device/driver.h>
#include <device/input/soo_mse.h>
#include <device/input/ps2.h>

#include <uapi/linux/input-event-codes.h>

/*
 * Maximal horizontal and vertical resolution of the display.
 * To be set via the ioctl SET_SIZE command.
 *
 * This is used to prevent the cursor from going outside the
 * display.
 */
static struct display_res {
	uint16_t h, v;
} res = { .h = 0xffff, .v = 0xffff };

/* Defines the mouse position and button states. */
struct ps2_mouse state = {
	.x = 0, .y = 0,
	.left = 0, .right = 0, .middle = 0
};

/* ioctl commands. */

#define GET_STATE 0
#define SET_SIZE  1

int ioctl_mouse(int fd, unsigned long cmd, unsigned long args);

/* Device info. */

struct file_operations vmse_fops = {
	.ioctl = ioctl_mouse
};

struct devclass vmse_cdev = {
	.class = DEV_CLASS_MOUSE,
	.type = VFS_TYPE_DEV_INPUT,
	.fops = &vmse_fops,
};

void so3virt_mse_event(unsigned int type, unsigned int code, int value)
{
	DBG("Input event: %u %u %d\n", type, code, value);

	if (type == EV_REL) {
		if (code == REL_X) {
			state.x = CLAMP(state.x + value, 0, res.h);
		}
		else if (code == REL_Y) {
			state.y = CLAMP(state.y + value, 0, res.v);
		}
	}
	else if (type == EV_ABS) {
		if (code == ABS_X) {
			state.x = value * res.h / 10000;
		}
		else if (code == ABS_Y) {
			state.y = value * res.v / 10000;
		}
	}
	else if (type == EV_KEY) {
		/*
		 * Here we only set the button states to "pressed". Their state
		 * will be changed to "released" once the state has been read,
		 * e.g. in the ioctl. So the client has the time to read the
		 * button states.
		 */
		if ((code == BTN_LEFT || code == BTN_TOUCH) && value) {
			state.left = value;
		}
		else if (code == BTN_MIDDLE && value) {
			state.middle = value;
		}
		else if (code == BTN_RIGHT && value) {
			state.right = value;
		}
	}

	DBG("xy[%04d, %04d]; %03s %03s %03s\n",
		state.x, state.y,
		state.left ? "LFT" : "", state.middle ? "MID" : "", state.right ? "RGT" : "");
}

int ioctl_mouse(int fd, unsigned long cmd, unsigned long args)
{
	switch (cmd) {

	case GET_STATE:
		/* Return the mouse coordinates and button states. */
		*((struct ps2_mouse *) args) = state;

		/* Reset the button states. */
		state.left = 0;
		state.right = 0;
		state.middle = 0;
		break;

	case SET_SIZE:
		/* Set the display maximum size. */
		res = *((struct display_res *) args);
		break;

	default:
		/* Unknown command. */
		return -1;
	}

	return 0;
}

int init_mouse(dev_t *dev)
{
	/* Register the input device so it can be accessed from user space. */
	devclass_register(dev, &vmse_cdev);
	return 0;
}

REGISTER_DRIVER_POSTCORE("mouse,so3virt", init_mouse);
