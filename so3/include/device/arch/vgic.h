/*
 * Copyright (C) 2024 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef VGIC_H
#define VGIC_H

#include <mmio.h>

#include <device/arch/gic.h>

#define REG_RANGE(base, n, size) (base)...((base) + (n - 1) * (size))

enum mmio_result gic_handle_dist_access(struct mmio_access *mmio);

#endif /* VGIC_H */
