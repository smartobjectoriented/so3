/*
 * Copyright (C) 2016-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef CONSOLE_H
#define CONSOLE_H

#include <vfs.h>

extern struct file_operations console_fops;

#ifdef CONFIG_AVZ

#include <avz/uapi/avz.h>

void console_init(void);
void console_init_post(void);

void printch(char c);

#endif

#endif /* CONSOLE_H */
