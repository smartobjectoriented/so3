/*
 * Copyright (C) 2020 Nikolaos Garanis <nikolaos.garanis@heig-vd.ch>
 * Copyright (C) 2021 Daniel Rossier <daniel.rossier@heig-vd.ch>
 *
 * With the kind support and contribution of Gabor Kiss-Vamosi from LVGL. Thank You!
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
 * demo.c
 *
 * A more complete LittlevGL demo.
 *
 * Based on:
 *  - https://github.com/littlevgl/lv_examples/blob/master/lv_tests/lv_test_theme/lv_test_theme_1.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <bits/ioctl_fix.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include <lvgl.h>

#include "lv_demo_stress.h"

/* Screen resolution. */
static uint32_t scr_hres, scr_vres, *fbp;

/* File descriptor of the mouse and keyboard input device. */
static int mfd;
static int kfd;

/* lvgl group for the keyboard. */
static lv_group_t *keyboard_group;

/* Used to measure the duration of execution */
struct timeval tv_start, tv_end;
static uint64_t delta;
static int fd;

/* Tick routine for lvgl. */
void *tick_routine(void *args)
{
	while (1) {
		/* Tell LittlevGL that 1 millisecond were elapsed */
		usleep(1000);
		lv_tick_inc(1);
	}
}

bool fs_ready_cb(struct _lv_fs_drv_t *drv)
{
	return true;
}

void *fs_open_cb(struct _lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
	FILE *fp = fopen(path, (mode & LV_FS_MODE_WR) ? "w" : "r");
	if (!fp) {
		return NULL;
	}

	return fp;
}

lv_fs_res_t fs_close_cb(struct _lv_fs_drv_t *drv, void *file_p)
{
	if (0 != fclose(file_p)) {
		return LV_FS_RES_UNKNOWN;
	}

	return LV_FS_RES_OK;
}

lv_fs_res_t fs_read_cb(struct _lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
	*br = fread(buf, sizeof(uint8_t), btr, file_p);
	return LV_FS_RES_OK;
}

lv_fs_res_t fs_seek_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
	if (0 != fseek(file_p, pos, SEEK_SET)) {
		return LV_FS_RES_UNKNOWN;
	}

	return LV_FS_RES_OK;
}

lv_fs_res_t fs_tell_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
	*pos_p = ftell(file_p);
	return LV_FS_RES_OK;
}

/* File system driver initialisation. */
void fs_init(void)
{
	static lv_fs_drv_t drv;
	lv_fs_drv_init(&drv);

	drv.letter = 'S';			/* An uppercase letter to identify the drive */
	drv.ready_cb = fs_ready_cb;		/* Callback to tell if the drive is ready to use */

	drv.open_cb = fs_open_cb;		/* Callback to open a file */
	drv.close_cb = fs_close_cb;		/* Callback to close a file */
	drv.read_cb = fs_read_cb;		/* Callback to read a file */
	drv.seek_cb = fs_seek_cb;		/* Callback to seek in a file (Move cursor) */
	drv.tell_cb = fs_tell_cb;		/* Callback to tell the cursor position */

	lv_fs_drv_register(&drv);
}




/*
 * Framebuffer callback. LVGL calls this function to redraw a screen area. If
 * the buffer given to LVGL is smaller than the framebuffer, this function will
 * be called multiple times until the whole screen has been redrawn. This is
 * why we cannot use memcpy to redraw the whole region, we have to do it line
 * by line.
 *
 * If the buffer is the size of the framebuffer, we could use memcpy for the
 * whole region, but then SO3 would require more memory.
 *
 * https://docs.lvgl.io/latest/en/html/porting/display.html#display-buffer
 */
void my_fb_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
	lv_coord_t y, w = lv_area_get_width(area);
	uint32_t line_size = w * sizeof(lv_color_t);

	for (y = area->y1; y <= area->y2; y++) {
		memcpy(&fbp[y * scr_hres + area->x1], color_p, line_size);
		color_p += w;
	}

	lv_disp_flush_ready(disp);
}

/*
 * Framebuffer and display initialisation.
 */
int fb_init(void)
{
	uint32_t fb_size;
	static lv_disp_drv_t disp_drv;

	/* LVGL will use this buffer to render the screen. See my_fb_cb. */
	static lv_color_t buf[LVGL_BUF_SIZE];
	static lv_disp_draw_buf_t disp_buf;

	/* Get file descriptor. */
	fd = open("/dev/fb0", 0);
	if (fd == -1) {
		printf("Couldn't open framebuffer.\n");
		return -1;
	}

	/* Get screen resolution. */
	if (ioctl(fd, IOCTL_FB_HRES, &scr_hres)
		|| ioctl(fd, IOCTL_FB_VRES, &scr_vres)
		|| ioctl(fd, IOCTL_FB_SIZE, &fb_size)) {

		printf("Couldn't get framebuffer resolution.\n");
		return -1;
	}

	/* Map the framebuffer into process memory. */
	fbp = mmap(NULL, fb_size, 0, 0, fd, 0);
	if (!fbp) {
		printf("Couldn't map framebuffer.\n");
		return -1;
	}

	lv_disp_draw_buf_init(&disp_buf, buf, NULL, LVGL_BUF_SIZE);

	/*
	 * Initialisation and registration of the display driver.
	 * Also setting the flush callback function (flush_cb) which will write
	 * the lvgl buffer (buf) into our real framebuffer.
	 */

	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res = scr_hres;
	disp_drv.ver_res = scr_vres;
	disp_drv.draw_buf = &disp_buf;
	disp_drv.flush_cb = my_fb_cb;

	lv_disp_drv_register(&disp_drv);

	return 0;
}

/* Main code. */

int main(int argc, char **argv)
{
	pthread_t tick_thread;

	/* Initialization of lvgl. */
	lv_init();
	fs_init();

	/* Initialization of the framebuffer, mouse and keyboard. */
	if (fb_init())
		return -1;

	/* Creating the UI. */
	//lv_demo_stress();

 	gettimeofday(&tv_start, NULL);
	lv_timer_handler();
	gettimeofday(&tv_end, NULL);

	delta = tv_end.tv_usec - tv_start.tv_usec;

	printf("Performance test result:\n\n");
	printf("# Elapsed time of lv_timer_handler() function: %lld microseconds.\n", delta);
	printf("\n***************************************************************************\n");
    close(fd);

#if 0
	/* LittlevGL needs to know how time passes by. */
	if (pthread_create(&tick_thread, NULL, tick_routine, NULL) == -1)
		return -1;

	/* LittlevGL has a set of tasks it needs to run every 5ms approx. */
	while (1) {
		lv_task_handler();
		usleep(5000);
	}
#endif

	return 0;
}
