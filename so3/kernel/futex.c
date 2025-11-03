/*
 * Copyright (C) 2025 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
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

#include <errno.h>
#include <futex.h>

SYSCALL_DEFINE6(futex, u32 *, uaddr, int, op, u32, val,
		const struct timespec *, utime,
		u32 *, uaddr2, u32, val3)
{

	// unsigned int flags = futex_to_flags(op);
	int cmd = op & FUTEX_CMD_MASK;

	switch (cmd) {
	case FUTEX_WAIT:
		break;
	case FUTEX_WAKE:
		break;
	default:
		printk("Futex cmd '%d' not supported !\n");
		return -EINVAL;
	}

	return -ENOSYS;
}



