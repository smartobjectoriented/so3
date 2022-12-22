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

#include <lvgl.h>
#include <lv_demo_widgets.h>

#include "demofb.h"

/* Screen resolution. */
static uint32_t scr_hres, scr_vres, *fbp;

/* File descriptor of the mouse and keyboard input device. */
static int mfd;
static int kfd;

/* lvgl group for the keyboard. */
static lv_group_t *keyboard_group;

/* Tick routine for lvgl. */
void *tick_routine(void *args)
{
	while (1) {
		/* Tell LittlevGL that 5 milliseconds were elapsed */
		usleep(1000);
		lv_tick_inc(1);
	}
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


/*
 * Framebuffer and display initialisation.
 */
int fb_init(void)
{
	int fd;
	uint32_t fb_size;
	static lv_disp_drv_t disp_drv;

	/* LVGL will use this buffer to render the screen. See my_fb_cb. */
	static lv_color_t buf[LVGL_BUF_SIZE];
	static lv_disp_draw_buf_t disp_buf;

	/* Get file descriptor. */
	fd = open("/dev/fb0", 0);
	if (-1 == fd) {
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

/* Mouse callback. */
void my_mouse_cb(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
	/* Retrieve mouse state from the driver. */
	static struct ps2_mouse mouse;

	ioctl(mfd, IOCTL_MOUSE_GET_STATE, &mouse);

	data->state = mouse.left ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

	data->point.x = mouse.x;
	data->point.y = mouse.y;
}

/*
 * Mouse initialisation.
 */
int mouse_init(void)
{
	struct display_res res = { .h = scr_hres, .v = scr_vres };
	static lv_indev_drv_t mouse_drv;

	mfd = open("/dev/mouse0", 0);
	if (mfd == -1) {
		printf("Couldn't open input device.\n");
		return -1;
	}

	/*
	 * Informing the driver of the screen size so it knows the bounds for
	 * the cursor coordinates.
	 */

	ioctl(mfd, IOCTL_MOUSE_SET_RES, &res);

	/*
	 * Initialisation of an input driver, in our case for a mouse. Also set
	 * a cursor image. We also set a callback function which will be called
	 * by lvgl periodically. This function queries the mouse driver for the
	 * xy coordinates and button states.
	 */
	lv_indev_drv_init(&mouse_drv);

	mouse_drv.type = LV_INDEV_TYPE_POINTER;
	mouse_drv.read_cb = my_mouse_cb;

	lv_indev_t *mouse_dev = lv_indev_drv_register(&mouse_drv);
	lv_obj_t *cursor_obj = lv_img_create(lv_disp_get_scr_act(NULL));
	lv_img_set_src(cursor_obj, LV_SYMBOL_PLUS);
	lv_indev_set_cursor(mouse_dev, cursor_obj);

	return 0;
}

/* Keyboard callback. */
void my_keyboard_cb(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
	/* Retrieve mouse state from the driver. */
	static struct ps2_key key;
	ioctl(kfd, IOCTL_KB_GET_KEY, &key);

	if (key.value != 0) {
		data->key = key.value;
		data->state = key.state & 1;
	}
}

/*
 * Keyboard initialisation.
 */
int keyboard_init(void)
{
	static lv_indev_drv_t keyboard_drv;
	lv_indev_t *keyboard_dev;

	kfd = open("/dev/keyboard0", 0);
	if (kfd == -1) {
		printf("Couldn't open input device.\n");
		return -1;
	}

	/**
	 * Initialisation of the keyboard input driver.
	 */

	lv_indev_drv_init(&keyboard_drv);
	keyboard_drv.type = LV_INDEV_TYPE_KEYPAD;
	keyboard_drv.read_cb = my_keyboard_cb;

	keyboard_dev = lv_indev_drv_register(&keyboard_drv);

	keyboard_group = lv_group_create();
	lv_indev_set_group(keyboard_dev, keyboard_group);

	return 0;
}

/* Main code. */

int main(int argc, char **argv)
{
	pthread_t tick_thread;

	/* Initialisation of lvgl. */
	lv_init();
	fs_init();

	/* Initialisation of the framebuffer, mouse and keyboard. */
	if (fb_init() || mouse_init() || keyboard_init()) {
		return -1;
	}

	/* Creating the UI. */
	lv_demo_widgets();

	/* LittlevGL needs to know how time passes by. */
	if (pthread_create(&tick_thread, NULL, tick_routine, NULL) == -1) {
		return -1;
	}

	/* LittlevGL has a set of tasks it needs to run every 5ms approx. */
	while (1) {
		lv_task_handler();
		usleep(5000);
	}

	return 0;
}
