
/*
 * Copyright (C) 2014-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef VERSION_H

/* CHANGES 2023.6.0 */
/**
 * - Removed CONFIG_RAM_BASE and CONFIG_RAM_SIZE. These information are available from the device tree.
 * - Removed old meminfo structure which is not used anymore.
 * - Removed avz_guest_phys_offset and rely on avz_shared to initialize mem_info structure (phys base + size)
 * - Upgrade U-boot to 2022.04
 */

#define SO3_KERNEL_VERSION "2023.6.0"

#endif /* VERSION_H */
