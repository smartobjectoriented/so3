/*
 * Copyright (C) 2026 REDS Institute, HEIG-VD, Yverdon
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
 * Minimal /dev/fb test program. It bypasses LVGL entirely: it opens the
 * framebuffer, queries its geometry through the same ioctls used by the slv
 * port (lib/slv/slv_fb.c), mmap()s the VRAM into the process and draws
 * directly into it. It paints static vertical colour bars plus an animated
 * square so it is obvious whether the display pipeline (PL111 CLCD -> QEMU
 * SDL window) works. Press Ctrl-C to quit.
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

/* Framebuffer device + ioctls, matching lib/slv/slv_fb.h and the kernel
 * drivers (devices/fb/pl111.c, virtfb.c). */
#define FB_DEV		"/dev/fb"
#define IOCTL_FB_HRES	1
#define IOCTL_FB_VRES	2
#define IOCTL_FB_SIZE	3
#define IOCTL_FB_BPP	5

/* Pack an 8-bit-per-channel colour for the framebuffer's pixel depth. */
static uint32_t pack(uint32_t bpp, uint8_t r, uint8_t g, uint8_t b)
{
	if (bpp == 16) /* RGB565 */
		return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);

	/* 24/32 bpp: one little-endian 0x00RRGGBB word per pixel. */
	return ((uint32_t) r << 16) | ((uint32_t) g << 8) | b;
}

/* Fill the rectangle [x0,x1[ x [y0,y1[ (clipped) with a packed colour. */
static void fill_rect(void *fbp, uint32_t hres, uint32_t vres, uint32_t bpp,
		      int x0, int y0, int x1, int y1, uint32_t color)
{
	if (x0 < 0)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (x1 > (int) hres)
		x1 = hres;
	if (y1 > (int) vres)
		y1 = vres;

	for (int y = y0; y < y1; y++) {
		if (bpp == 16) {
			uint16_t *row = (uint16_t *) fbp + (size_t) y * hres;
			for (int x = x0; x < x1; x++)
				row[x] = (uint16_t) color;
		} else {
			uint32_t *row = (uint32_t *) fbp + (size_t) y * hres;
			for (int x = x0; x < x1; x++)
				row[x] = color;
		}
	}
}

int main(int argc, char **argv)
{
	uint32_t hres, vres, fb_size, bpp;
	int fd;
	void *fbp, *bg;

	fd = open(FB_DEV, 0);
	if (fd < 0) {
		printf("fb_test: cannot open %s\n", FB_DEV);
		return EXIT_FAILURE;
	}

	if (ioctl(fd, IOCTL_FB_HRES, &hres) || ioctl(fd, IOCTL_FB_VRES, &vres) ||
	    ioctl(fd, IOCTL_FB_SIZE, &fb_size) || ioctl(fd, IOCTL_FB_BPP, &bpp)) {
		printf("fb_test: cannot query framebuffer geometry\n");
		return EXIT_FAILURE;
	}

	printf("fb_test: %dx%d, %d bpp, %d bytes\n", hres, vres, bpp, fb_size);

	if (bpp != 16 && bpp != 32) {
		printf("fb_test: unsupported bpp %d\n", bpp);
		return EXIT_FAILURE;
	}

	fbp = mmap(NULL, fb_size, 0, 0, fd, 0);
	if (fbp == MAP_FAILED) {
		printf("fb_test: mmap failed\n");
		return EXIT_FAILURE;
	}

	/* Build a static background (colour bars + white border) once into a
	 * private buffer, so each animation frame is a cheap memcpy + square. */
	bg = malloc(fb_size);
	if (!bg) {
		printf("fb_test: out of memory\n");
		return EXIT_FAILURE;
	}

	static const uint8_t bars[8][3] = {
		{ 0, 0, 0 },     { 255, 0, 0 },   { 0, 255, 0 },   { 0, 0, 255 },
		{ 255, 255, 0 }, { 255, 0, 255 }, { 0, 255, 255 }, { 255, 255, 255 },
	};
	for (int i = 0; i < 8; i++)
		fill_rect(bg, hres, vres, bpp, i * hres / 8, 0,
			  (i + 1) * hres / 8, vres,
			  pack(bpp, bars[i][0], bars[i][1], bars[i][2]));

	/* 4-pixel white border. */
	uint32_t white = pack(bpp, 255, 255, 255);
	fill_rect(bg, hres, vres, bpp, 0, 0, hres, 4, white);
	fill_rect(bg, hres, vres, bpp, 0, vres - 4, hres, vres, white);
	fill_rect(bg, hres, vres, bpp, 0, 0, 4, vres, white);
	fill_rect(bg, hres, vres, bpp, hres - 4, 0, hres, vres, white);

	printf("fb_test: drawing colour bars + moving square, Ctrl-C to quit\n");

	const int sq = 120;
	int x = 0, dx = 6;
	uint32_t orange = pack(bpp, 255, 140, 0);
	int y = (vres - sq) / 2;

	while (1) {
		/* Repaint the background, then the square at its new position. */
		memcpy(fbp, bg, fb_size);
		fill_rect(fbp, hres, vres, bpp, x, y, x + sq, y + sq, orange);

		x += dx;
		if (x <= 0 || x + sq >= (int) hres)
			dx = -dx;

		usleep(16000); /* ~60 fps */
	}

	return EXIT_SUCCESS;
}
