/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
 * Low-level ARM-specific setup
 */

#include <memory.h>
#include <types.h>

#include <device/fdt.h>

/*
 * Low-level initialization before the main boostrap process.
 */
void setup_arch(void) {
	int offset;

	/* Access to device tree */
	offset = get_mem_info((void *) __fdt_addr, &mem_info);
	if (offset >= 0)
		DBG("Found %d MB of RAM at 0x%08X\n", mem_info.size / SZ_1M, mem_info.phys_base);

	/* A low-level UART should be initialized here so that subsystems initialization (like MMC) can already print out logs ... */
}
