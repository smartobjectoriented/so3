#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <sys/types.h>
#include <fcntl.h>

#define MASK5 0x1f
#define MASK6 0x3f

uint16_t create_px(uint16_t r, uint16_t g, uint16_t b);

/*
 * Demo application to show how to open a framebuffer file type, use mmap to
 * access its framebuffer and write into it.
 */
int main(int argc, char **argv)
{
	int fd;
	uint32_t i, j;
	uint16_t* vram_addr;

	/* Get file descriptor of the fb.0 framebuffer, i.e. the first fb debice registered. */
	fd = open("fb.0", 0);

	/* Map the framebuffer memory (VRAM) to a process virtual address. */
	/* TODO place this syscall in a wrapper? */
	vram_addr = sys_mmap(0x4b000 * 2, 0, fd, 0);
	printf("ptr: 0x%08x\n", vram_addr);

	/* Display some pixels. */

	for (i = 0; i < 160; i++) {
		for (j = 0; j < 640; j++) {
			vram_addr[j + i * 640] = create_px(0xff, 0, 0);
		}
	}

	vram_addr += 160 * 640;
	for (i = 0; i < 160; i++) {
		for (j = 0; j < 640; j++) {
			vram_addr[j + i * 640] = create_px(0, 0xff, 0);
		}
	}

	vram_addr += 160 * 640;
	for (i = 0; i < 160; i++) {
		for (j = 0; j < 640; j++) {
			vram_addr[j + i * 640] = create_px(0, 0, 0xff);
		}
	}

	return 0;
}

uint16_t create_px(uint16_t r, uint16_t g, uint16_t b)
{
	/* Depends on the driver's bpp mode, here for bgr565. */
	uint16_t px = 0;
	px |= r & MASK5;
	px |= (g & MASK6) << 5;
	px |= (b & MASK5) << 11;
	return px;
}
