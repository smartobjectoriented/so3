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
 * lvgl.c
 *
 * The purpose of this file is to build a simple LittlevGL application to make
 * sure the integration of lvgl is working properly.
 *
 * Documentation and sources:
 *  - https://github.com/littlevgl/lv_port_linux_frame_buffer/blob/master/main.c
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "lvgl/lvgl.h"

#define BUF_SIZE (LV_HOR_RES_MAX * LV_VER_RES_MAX)
#define FB_SIZE (BUF_SIZE * LV_COLOR_DEPTH / 8)

/* Pointer on the framebuffer. */
static uint16_t *fbp;

int fb_init(void);
void create_ui(void);
void flush_cb(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p);
void btn_event_cb(lv_obj_t * btn, lv_event_t event);


int main(int argc, char **argv)
{
	/* Initialisation of lvgl (internal stuff). */
	lv_init();

	/* Initialisation of our framebuffer. */
	if (fb_init()) {
		printf("Framebuffer initialisation failed.\n");
	}

	/* lvgl will draw the screen into this buffer. */
	static lv_color_t buf[BUF_SIZE];

	/* Initialisation a buffer descriptor. */
	static lv_disp_buf_t disp_buf;
	lv_disp_buf_init(&disp_buf, buf, NULL, BUF_SIZE);

	/*
	 * Initialisation and registration of the display driver.
	 * Also setting the flush callback function (flush_cb) which will write
	 * the lvgl buffer (buf) into our real framebuffer.
	 */
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.buffer = &disp_buf;
	disp_drv.flush_cb = flush_cb;
	lv_disp_drv_register(&disp_drv);

	/* Creating a simple UI. */
	create_ui();

	/* lvgl tasks need to run every 5ms approx. */
	while (1) {
		lv_task_handler();
		usleep(5000);
	}

	return 0;
}

int fb_init()
{
	int fd;

	/* Get file descriptor. */
	fd = open("fb.0", 0);
	if (-1 == fd) {
		printf("Couldn't open framebuffer.\n");
		return -1;
	}

	/* Map the fb into process memory. */
	fbp = mmap(NULL, FB_SIZE, 0, 0, fd, 0);
	if (!fbp) {
		printf("Couldn't map framebuffer.\n");
		return -1;
	}

	return 0;
}

void flush_cb(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p)
{
	int32_t x, y;
	for(y = area->y1; y <= area->y2; y++) {
		for(x = area->x1; x <= area->x2; x++) {
			fbp[x + y * disp->hor_res] = color_p->full;
			color_p++;
		}
	}

	lv_disp_flush_ready(disp);
}

void create_ui(void)
{
	lv_obj_t * btn = lv_btn_create(lv_scr_act(), NULL); /* Add a button the current screen */
	lv_obj_set_pos(btn, 10, 10);                        /* Set its position */
	lv_obj_set_size(btn, 100, 50);                      /* Set its size */
	lv_obj_set_event_cb(btn, btn_event_cb);             /* Assign a callback to the button */

	lv_obj_t * label = lv_label_create(btn, NULL);      /* Add a label to the button */
	lv_label_set_text(label, "Button");                 /* Set the labels text */
}

void btn_event_cb(lv_obj_t * btn, lv_event_t event)
{
	if(event == LV_EVENT_CLICKED) {
		printf("Clicked!\n");
	}
}
