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

/*
 * These definitions are heavly borrowed from jailhouse hypervisor
 */

#ifndef MMIO_H
#define MMIO_H

#include <types.h>

#include <asm/processor.h>

/** MMIO access result. */
enum mmio_result { MMIO_ERROR = -1, MMIO_UNHANDLED, MMIO_HANDLED };

/** MMIO access description. */
struct mmio_access {
	/** Address to access, depending on the context, an absolute address or
	 * relative offset to region start. */
	unsigned long address;

	/** Size of the access. */
	unsigned int size;

	/** True if write access. */
	bool is_write;

	/** The value to be written or the read value to return. */
	unsigned long value;
};

int mmio_dabt_decode(cpu_regs_t *regs, unsigned long esr);
void mmio_perform_access(void *base, struct mmio_access *mmio);

#endif /* MMIO_H */
