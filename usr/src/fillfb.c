#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>

#define H_RES 1024
#define V_RES 768
#define FB_SIZE (H_RES * V_RES * 4)

#define MASK5 0x1f
#define MASK6 0x3f

uint32_t create_px(uint8_t r, uint8_t g, uint8_t b);

/*
 * Demo application to show how to open a framebuffer file type, use mmap to
 * access its framebuffer and write into it.
 */
int main(int argc, char **argv)
{
	int fd;
	uint32_t i, j;
	uint32_t* fbp;

	/* Get file descriptor for /dev/fb0, i.e. the first fb device registered. */
	fd = open("/dev/fb0", 0);

	/* Map the framebuffer memory to a process virtual address. */
	fbp = mmap(NULL, FB_SIZE, 0, 0, fd, 0);

	/* Display some pixels. */

	for (i = 0; i < V_RES / 3; i++) {
		for (j = 0; j < H_RES; j++) {
			fbp[j + i * H_RES] = create_px(0xff, 0, 0);
		}
	}

	fbp += V_RES / 3 * H_RES;
	for (i = 0; i < V_RES / 3; i++) {
		for (j = 0; j < H_RES; j++) {
			fbp[j + i * H_RES] = create_px(0, 0xff, 0);
		}
	}

	fbp += V_RES / 3 * H_RES;
	for (i = 0; i < V_RES / 3; i++) {
		for (j = 0; j < H_RES; j++) {
			fbp[j + i * H_RES] = create_px(0, 0, 0xff);
		}
	}

	return 0;
}

uint32_t create_px(uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t px = 0;
	px |= b;
	px |= g << 8;
	px |= r << 16;
	return px;
}
