/*
 * Copyright (C) 2016-2025 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef AVZ_CONSOLE_H
#define AVZ_CONSOLE_H

#include <stdarg.h>
#include <compiler.h>

#include <avz/uapi/avz.h>

void init_console(void);

extern void (*__printch)(char c);

void lprintk(char *format, ...) __attribute_printf(1, 2);
void lprintk_buffer(void *buffer, uint32_t n);
void lprintk_buffer_separator(void *buffer, uint32_t n, char separator);

/* Used to print out to the syslog file */
void printk_buffer(void *buffer, uint32_t n);

void __lprintk(const char *format, va_list va);

void lprintch(char c);

void lprintk_int64_post(s64 number, char *post);
void lprintk_int64(s64 number);

void do_console_io(console_t *console);

#endif /* AVZ_CONSOLE_H */
