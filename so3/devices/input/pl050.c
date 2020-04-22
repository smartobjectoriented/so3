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
 * This is a driver for the PL050. The PL050 is a controller for both the mouse
 * and the keyboard. The differences are the base addresses and the interrupt
 * numbers.
 *
 * We currently only use it for the mouse.
 *
 * Documentation:
 *   http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0143c/index.html
 *   https://wiki.osdev.org/PL050_PS/2_Controller
 */

#include <printk.h>
#include <vfs.h>
#include <asm/io.h>
#include <device/driver.h>
#include <device/irq.h>
#include <device/input/ps2.h>

/* Register address offsets */
#define KMI_CR     0x00
#define KMI_STAT   0x04
#define KMI_DATA   0x08
#define KMI_CLKDIV 0x0c
#define KMI_IR     0x10

/* Control register values */
#define KMICR_TYPE     (0 << 5) /* PS2/AT mode */
#define KMICR_RXINTREN (1 << 4) /* enable receiver interrupt */
#define KMICR_TXINTREN (0 << 3) /* disable transmitter interrupt */
#define KMICR_EN       (1 << 2) /* enable interface */
#define KMICR_FD       (0 << 1)
#define KMICR_FC       (0 << 0)

/* Status register values */
#define KMISTAT_TXEMPTY  (1 << 6) /* transmit register is empty, can be written */
#define KMISTAT_TXBUSY   (1 << 5) /* data is being sent */
#define KMISTAT_RXFULL   (1 << 4) /* receive register is full, can be read */
#define KMISTAT_RXBUSY   (1 << 3) /* data is being received */
#define KMISTAT_RXPARITY (1 << 2) /* parity of last received byte */
#define KMISTAT_IC       (1 << 1)
#define KMISTAT_ID       (1 << 0)

/* Interrupt status register values */
#define KMIIR_TXINTR (1 << 1) /* transmitter interrupt asserted */
#define KMIIR_RXINTR (1 << 0) /* receiver interrupt asserted */

/* ioctl commands. */
#define GET_STATE 0
#define SET_SIZE  1

int ioctl(int fd, unsigned long cmd, unsigned long args);


dev_t pl050_dev;

/*
 * Maximal horizontal and vertical resolution of the display.
 * To be set via the ioctl SET_SIZE command.
 *
 * This is used to prevent the cursor from going outside the
 * display.
 */
struct display_res {
	uint16_t h, v;
} res = { .h = 0xffff, .v = 0xffff };

/* Defines the mouse position and button states. */
struct mouse_state state = {
	.x = 0, .y = 0,
	.left = 0, .right = 0, .middle = 0
};

struct file_operations pl050_fops = {
	.ioctl = ioctl
};

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
irq_return_t pl050_int(int irq, void *dummy)
{
	uint8_t status, packet[3], i, tmp;

	/* Read the interrupt status register. */
	status = ioread8(pl050_dev.base + KMI_IR);

	/* As long as a receiver interrupt has been asserted… */
	i = 0;
	while (status & KMIIR_RXINTR) {

		/*
		 * Read from the data register. We care only about the first 3
		 * bytes, but we must continue reading until there are none
		 * (otherwise the status value will not change).
		 */
		tmp = ioread8(pl050_dev.base + KMI_DATA);
		if (i < 3) {
			packet[i++] = tmp;
		}

		/* Update status. */
		status = ioread8(pl050_dev.base + KMI_IR);
	}

	/* Set mouse coordinates and button states. */
	if (i == 3) {
		get_ps2_state(packet, &state, res.h, res.v);
	}

	return IRQ_COMPLETED;
}

/* Write a byte to the device. */
void pl050_write(uint8_t data)
{
	/* Check if we can actually write in the transmit registry. */
	uint8_t status = ioread8(pl050_dev.base + KMI_STAT);
	if (0 == (status & KMISTAT_TXEMPTY)) {
		printk("%s: write timeout.\n", __func__);
		return;
	}

	iowrite8(pl050_dev.base + KMI_DATA, data);
}

/*
 * Initialisation of the PL050 Keyboard/Mouse Interface.
 * Linux driver: input/serio/ambakmi.c
 */
int pl050_init(dev_t *dev)
{
	/* Keep a reference to the device structure. */
	memcpy(&pl050_dev, dev, sizeof(dev_t));

	/* Set the clock divisor (arbitrary value). */
	iowrite8(dev->base + KMI_CLKDIV, 5);

	/* Enable interface and receiver interrupt. */
	iowrite8(dev->base + KMI_CR, KMICR_EN | KMICR_RXINTREN | KMICR_TXINTREN);

	/* Bind the ISR to the interrupt controller. */
	irq_bind(dev->irq, pl050_int, NULL, NULL);

	/* Register the input device so it can be accessed from user space. */
	dev_register(&pl050_rdev);

	/* Tell the mouse to send PS/2 packets when moving. */
	pl050_write(EN_PKT_STREAM);

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

REGISTER_DRIVER_POSTCORE("arm,pl050", pl050_init);
