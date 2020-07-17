/*
 * Copyright (C) 2020 Nikolaos Garanis <nikolaos.garanis@heig-vd.ch>
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
 * Demo application to show how to open a framebuffer file, use mmap to access
 * its framebuffer and write into it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <bits/ioctl_fix.h>

/* Framebuffer ioctl commands. */
#define IOCTL_FB_HRES 1
#define IOCTL_FB_VRES 2
#define IOCTL_FB_SIZE 3


static uint32_t hres;

void display_line(uint32_t *fbp, uint32_t start, uint32_t px)
{
	uint32_t i;
	for (i = 0; i < hres; i++)
		fbp[start + i] = px;
}

uint32_t create_px(uint8_t r, uint8_t g, uint8_t b)
{
	return (b << 16) | (g << 8) | r; /* 24bpp BGR mode */
}

int main(int argc, char **argv)
{
	int fd;
	uint32_t i, px, vres, fb_size, *fbp;
	uint32_t colors[] = {
		create_px(0xff, 0, 0),
		create_px(0, 0xff, 0),
		create_px(0, 0, 0xff)
	};

	/* Get file descriptor for /dev/fb0, i.e. the first fb device registered. */
	fd = open("/dev/fb0", O_WRONLY);

	ioctl(fd, IOCTL_FB_HRES, &hres);
	ioctl(fd, IOCTL_FB_VRES, &vres);
	ioctl(fd, IOCTL_FB_SIZE, &fb_size);

	/* Map the framebuffer memory to a process virtual address. */
	fbp = mmap(NULL, fb_size, 0, 0, fd, 0);

	/* Display lines of different colors. */
	for (i = 0; i < vres; i++) {
		px = colors[(i / 12) % 3];
		display_line(fbp, i * hres, px);
	}

	close(fd);
	return EXIT_SUCCESS;
}
