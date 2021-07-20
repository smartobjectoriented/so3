/*
 * Copyright (C) 2021      Nicolas Müller <nicolas.muller1@heig-vd.ch>
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

.section ".vectors.text", "ax"

.globl _vectors

.extern _start

/* RISC-V uses one address that handles every trap. There is no need for a vector table.
 * Note: There is an option to do it with a vector table if really needed. */
_vectors:
    # reset
    j   _start
