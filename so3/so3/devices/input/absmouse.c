/*
 * Copyright (C) 2026 REDS Institute, HEIG-VD, Yverdon
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
 * Driver for the SO3 absolute pointer ("so3,absmouse").
 *
 * This is the polled MMIO device added to the so3 QEMU virt machine (see
 * hw/arm/virt.c, VirtAbsMouseState). Unlike the relative PL050 PS/2 mouse
 * (kmi1.c), it reports ABSOLUTE coordinates fed by QEMU's absolute input
 * handler, so the guest cursor maps 1:1 onto the host pointer with no
 * grab/warp and no host/guest drift (works on Wayland and across monitors).
 *
 * Registers (read-only):
 *   0x00  X       0 .. MAX   (QEMU absolute range)
 *   0x04  Y       0 .. MAX
 *   0x08  BTN     bit0 left, bit1 right, bit2 middle
 *   0x0c  MAX     INPUT_EVENT_ABS_MAX (scale reference)
 *
 * No IRQ: the user-space mouse read-cb polls GET_STATE every LVGL refresh.
 * We register DEV_CLASS_MOUSE (i.e. /dev/mouse); the PL050 mouse dts node is
 * disabled when this device is present so there is a single /dev/mouse.
 */

#include <vfs.h>
#include <memory.h>

#include <asm/io.h>

#include <device/driver.h>
#include <device/input/ps2.h>

/* MMIO register offsets (must match hw/arm/virt.c). */
#define ABSMOUSE_REG_X   0x00
#define ABSMOUSE_REG_Y   0x04
#define ABSMOUSE_REG_BTN 0x08
#define ABSMOUSE_REG_MAX 0x0c

/* ioctl commands (must match kmi1.c and usr/lib/slv/slv_mouse.h). */
#define GET_STATE 0
#define SET_SIZE  1

/* Display resolution, set via SET_SIZE; used to scale the absolute range. */
struct display_res {
	uint16_t h, v;
};

static struct display_res res = { .h = 0, .v = 0 };
static void *absmouse_base;

static int ioctl_absmouse(int fd, unsigned long cmd, unsigned long args);

static struct file_operations absmouse_fops = { .ioctl = ioctl_absmouse };

static struct devclass absmouse_cdev = {
	.class = DEV_CLASS_MOUSE,
	.type = VFS_TYPE_DEV_INPUT,
	.fops = &absmouse_fops,
};

static int absmouse_init(dev_t *dev, int fdt_offset)
{
	const struct fdt_property *prop;
	int prop_len;

	printk("%s: probing the SO3 absolute pointer (so3,absmouse)...\n", __func__);

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);
	BUG_ON(prop_len != 2 * sizeof(unsigned long));

#ifdef CONFIG_ARCH_ARM32
	absmouse_base = (void *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]),
					fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
#else
	absmouse_base = (void *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]),
					fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));
#endif

	/* Register the input device so it can be accessed from user space. */
	devclass_register(dev, &absmouse_cdev);

	return 0;
}

static int ioctl_absmouse(int fd, unsigned long cmd, unsigned long args)
{
	struct ps2_mouse *out;
	uint32_t rawx, rawy, btn, absmax;

	switch (cmd) {
	case GET_STATE:
		out = (struct ps2_mouse *) args;

		rawx = ioread32(absmouse_base + ABSMOUSE_REG_X);
		rawy = ioread32(absmouse_base + ABSMOUSE_REG_Y);
		btn = ioread32(absmouse_base + ABSMOUSE_REG_BTN);
		absmax = ioread32(absmouse_base + ABSMOUSE_REG_MAX);
		if (absmax == 0)
			absmax = 0x7fff;

		/* Scale QEMU's absolute range [0..absmax] to [0..res-1]. */
		out->x = (res.h > 0) ? (int16_t) (((uint64_t) rawx * (res.h - 1)) / absmax) : 0;
		out->y = (res.v > 0) ? (int16_t) (((uint64_t) rawy * (res.v - 1)) / absmax) : 0;

		out->left = (btn & 1) ? 1 : 0;
		out->right = (btn & 2) ? 1 : 0;
		out->middle = (btn & 4) ? 1 : 0;

		break;

	case SET_SIZE:
		res = *((struct display_res *) args);
		break;

	default:
		return -1;
	}

	return 0;
}

REGISTER_DRIVER_POSTCORE("so3,absmouse", absmouse_init);
