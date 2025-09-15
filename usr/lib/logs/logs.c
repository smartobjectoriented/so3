/*
 * Copyright (C) 2024 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
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

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>



void logs(const char *fmt, ...)
{
	int fd;
	va_list       args;
	static char   buffer[1024];

	va_start(args, fmt);
	(void)vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	fd = open("/dev/logsdev", O_RDWR);
	write(fd, buffer, strlen(buffer) + 1);
	close(fd);
}

// void logs(const char *fmt, ...)
// {
// 	static char   buf[1024];
// 	static char   msg[1024];
// 	va_list       args;
// 	char         *p, *q;
// 	ME_desc_t     *capsule_desc;



// 	capsule_desc = get_ME_desc();

// 	sprintf(msg, "[ME:%d] %s", capsule_desc->slotID, buf);

// 	p = msg;

// 	while ((q = strchr(p, '\n')) != NULL) {
// 		*q = '\0';

// 		logs_serial_write(p, strlen(p) + 1);
// 		logs_serial_write("\n", 2);

// 		p = q + 1;
// 	}

// 	if (*p != '\0')
// 		logs_serial_write(p, strlen(p) + 1);
// }


