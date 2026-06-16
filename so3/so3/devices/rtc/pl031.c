/*
 * Copyright (C) 2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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
 * Driver for the ARM PL031 real-time clock (the QEMU virt machine exposes one
 * at 0x09010000, seeded from the host's wall-clock time). We only need to read
 * the time, so the driver simply maps the device and exposes rtc_get_time();
 * the Data Register (offset 0x00) reads back the current count of seconds since
 * the Unix epoch.
 */

#include <vfs.h>
#include <memory.h>

#include <asm/io.h>

#include <device/driver.h>
#include <device/rtc.h>

#define PL031_DR 0x00 /* Data Register: current time, seconds since epoch */

static void *pl031_base;

uint32_t rtc_get_time(void)
{
	if (!pl031_base)
		return 0;

	return ioread32(pl031_base + PL031_DR);
}

static int pl031_init(dev_t *dev, int fdt_offset)
{
	const struct fdt_property *prop;
	int prop_len;

	(void) dev;

	printk("%s: probing the PL031 real-time clock...\n", __func__);

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);
	BUG_ON(prop_len != 2 * sizeof(unsigned long));

#ifdef CONFIG_ARCH_ARM32
	pl031_base = (void *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]),
				     fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
#else
	pl031_base = (void *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]),
				     fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));
#endif

	return 0;
}

REGISTER_DRIVER_POSTCORE("arm,pl031", pl031_init);
