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

#include <string.h>
#include <common.h>
#include <stdarg.h>
#include <process.h>
#include <vfs.h>

#include <device/serial.h>

/*
 * Standard version of printk to be used.
 */
void printk(const char *fmt, ...)
{
	static char   buf[1024];

	va_list       args;
	char         *p, *q;

	va_start(args, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	p = buf;

	while ((q = strchr(p, '\n')) != NULL)
	{
		*q = '\0';

		serial_write(p, strlen(p)+1);
		serial_write("\n", 2);

		p = q + 1;
	}

	if (*p != '\0')
		serial_write(p, strlen(p)+1);

}

