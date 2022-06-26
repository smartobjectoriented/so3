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
 * Driver for the mouse (see pl050.c).
 */

#include <vfs.h>
#include <memory.h>

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
struct ps2_mouse state = {
	.x = 0, .y = 0,
	.left = 0, .right = 0, .middle = 0
};

/* ioctl commands. */
#define GET_STATE 0
#define SET_SIZE  1

int ioctl_mouse(int fd, unsigned long cmd, unsigned long args);

struct file_operations pl050_mouse_fops = {
	.ioctl = ioctl_mouse
};

/* Device info. */

struct devclass pl050_mouse_cdev = {
	.class = DEV_CLASS_MOUSE,
	.type = VFS_TYPE_DEV_INPUT,
	.fops = &pl050_mouse_fops,
};

struct {
	void *base;
	irq_def_t irq_def;
} pl050_mouse;

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
	if (i == 3)
		get_mouse_state(packet, &state, res.h, res.v);

	return IRQ_COMPLETED;
}

int pl050_init_mouse(dev_t *dev, int fdt_offset)
{
	const struct fdt_property *prop;
	int prop_len;

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);
	BUG_ON(prop_len != 2 * sizeof(unsigned long));

	/* Mapping the two mem area of GIC (distributor & CPU interface) */
#ifdef CONFIG_ARCH_ARM32
	pl050_mouse.base = (void *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]), fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
#else
	pl050_mouse.base = (void *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]), fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));
#endif

	fdt_interrupt_node(fdt_offset, &pl050_mouse.irq_def);

	/* Register the input device so it can be accessed from user space. */
	devclass_register(dev, &pl050_mouse_cdev);

	pl050_init(pl050_mouse.base, &pl050_mouse.irq_def, pl050_int_mouse);

	/* Tell the mouse to send PS/2 packets when moving. */
	pl050_write(pl050_mouse.base, EN_PKT_STREAM);

	return 0;
}

int ioctl_mouse(int fd, unsigned long cmd, unsigned long args)
{
	switch (cmd) {

	case GET_STATE:
		/* Return the mouse coordinates and button states. */
		*((struct ps2_mouse *) args) = state;

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

REGISTER_DRIVER_POSTCORE("arm,pl050,mouse", pl050_init_mouse);
