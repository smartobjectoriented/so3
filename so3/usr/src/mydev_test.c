/*
 * Copyright (C) 2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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

/* Smoke test for the /dev/mydev character device: write a string to it, read it
 * back, and assert the two buffers match. */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

int main(void)
{
	char write_buffer[20];
	char read_buffer[20];

	int fd = open("/dev/mydev", O_RDWR);

	sprintf(write_buffer, "Hello My Dev!");
	write(fd, write_buffer, strlen(write_buffer) + 1);

	read(fd, read_buffer, sizeof(read_buffer));

	printf("Wrote: %s - Read %s\n", write_buffer, read_buffer);
	assert(!strcmp(write_buffer, read_buffer));
	return 0;
}
