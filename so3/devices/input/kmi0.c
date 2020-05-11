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
 * Driver for the keyboard (see pl050.c).
 */

#if 0
#define DEBUG
#endif

#include <vfs.h>
#include <common.h>
#include <asm/io.h>
#include <device/driver.h>
#include <device/input/ps2.h>
#include <device/input/pl050.h>


/* ioctl commands. */
#define GET_KEY 0

int ioctl_keyboard(int fd, unsigned long cmd, unsigned long args);

struct file_operations pl050_keyboard_fops = {
	.ioctl = ioctl_keyboard
};

/* Device info. */

dev_t pl050_keyboard;

struct devclass pl050_keyboard_cdev = {
	.class = DEV_CLASS_INPUT,
	.type = VFS_TYPE_DEV_INPUT,
	.fops = &pl050_keyboard_fops,
};

struct ps2_key last_key = {
	.value = 0,
	.state = 0
};

/*
 * Keyboard interrupt service routine.
 */
irq_return_t pl050_int_keyboard(int irq, void *dummy)
{
	uint8_t status, packet[2], i, tmp;

	/* Read the interrupt status register. */
	status = ioread8(pl050_keyboard.base + KMI_IR);

	/* As long as a receiver interrupt has been asserted… */
	i = 0;
	DBG("---- Scan codes:\n");
	while (status & KMIIR_RXINTR) {

		/*
		 * Read from the data register. We care only about the first 2
		 * bytes but we must continue reading until there are none
		 * (otherwise the status register will not change).
		 *
		 * The number of scan codes can go up to at least 8 (pause
		 * pressed) but we handle only the most simple cases.
		 */
		tmp = ioread8(pl050_keyboard.base + KMI_DATA);
		if (i < 2) {
			packet[i++] = tmp;
		}

		DBG("0x%02x\n", tmp);

		/* Update status. */
		status = ioread8(pl050_keyboard.base + KMI_IR);
	}

	get_kb_key(packet, i, &last_key);

	return IRQ_COMPLETED;
}

int pl050_init_keyboard(dev_t *dev)
{
	return pl050_init(dev, &pl050_keyboard, &pl050_keyboard_cdev, pl050_int_keyboard);
}

int ioctl_keyboard(int fd, unsigned long cmd, unsigned long args)
{
	switch (cmd) {

	case GET_KEY:
		*((struct ps2_key *) args) = last_key;
		/* Indicate we have read the key and don't need to read it again. */
		last_key.value = 0;
		break;

	default:
		/* Unknown command. */
		return -1;
	}

	return 0;
}

REGISTER_DRIVER_POSTCORE("arm,pl050,keyboard", pl050_init_keyboard);
