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

#include <vfs.h>
#include <asm/io.h>
#include <device/driver.h>
#include <device/input/ps2.h>
#include <device/input/pl050.h>


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
static struct mouse_state state = {
	.x = 0, .y = 0,
	.left = 0, .right = 0, .middle = 0
};

/* ioctl commands. */
#define GET_STATE 0
#define SET_SIZE  1

int ioctl(int fd, unsigned long cmd, unsigned long args);

 struct file_operations pl050_fops = {
	.ioctl = ioctl
};

/* Device info. */
static dev_t pl050_mouse;
 struct reg_dev pl050_rdev = {
	.class = DEV_CLASS_INPUT,
	.type = VFS_TYPE_INPUT,
	.fops = &pl050_fops,
	.list = LIST_HEAD_INIT(pl050_rdev.list)
};

/*
 * Mouse interrupt service routine.
 *
 * Called each time the mouse sends a packet. We use the packet to compute the
 * mouse coordinates and retrieve its button states.
 */
irq_return_t pl050_int_mouse(int irq, void *dummy)
{
	uint8_t status, packet[3], i, tmp;

	/* Read the interrupt status register. */
	status = ioread8(pl050_mouse.base + KMI_IR);

	/* As long as a receiver interrupt has been asserted… */
	i = 0;
	while (status & KMIIR_RXINTR) {

		/*
		 * Read from the data register. We care only about the first 3
		 * bytes, but we must continue reading until there are none
		 * (otherwise the status value will not change).
		 */
		tmp = ioread8(pl050_mouse.base + KMI_DATA);
		if (i < 3) {
			packet[i++] = tmp;
		}

		/* Update status. */
		status = ioread8(pl050_mouse.base + KMI_IR);
	}

	/* Set mouse coordinates and button states. */
	if (i == 3) {
		get_ps2_state(packet, &state, res.h, res.v);
	}

	return IRQ_COMPLETED;
}

/*
 * Initialisation of the PL050 Keyboard/Mouse Interface.
 * Linux driver: input/serio/ambakmi.c
 */
int pl050_init_mouse(dev_t *dev)
{
	pl050_init(dev, &pl050_mouse, &pl050_rdev, pl050_int_mouse);

	/* Tell the mouse to send PS/2 packets when moving. */
	pl050_write(&pl050_mouse, EN_PKT_STREAM);

	return 0;
}

/* Mouse ioctl. */
int ioctl(int fd, unsigned long cmd, unsigned long args)
{
	static struct mouse_state *s;
	static struct display_res *r;

	switch (cmd) {

	case GET_STATE:
		/* Return the mouse coordinates and button states. */
		s = (struct mouse_state *) args;
		*s = state;

		/* Reset the button states. */
		state.left = 0;
		state.right = 0;
		state.middle = 0;
		break;

	case SET_SIZE:
		/* Set the display maximum size. */
		r = (struct display_res *) args;
		res = *r;
		break;

	default:
		/* Unknown command. */
		return -1;
	}

	return 0;
}

REGISTER_DRIVER_POSTCORE("arm,pl050,mouse", pl050_init_mouse);
