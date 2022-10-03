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

#include <lvgl.h>

#define LVGL_BUF_SIZE (256 * 256)

/* Framebuffer ioctl commands. */

#define IOCTL_FB_HRES 1
#define IOCTL_FB_VRES 2
#define IOCTL_FB_SIZE 3

/* Function prototypes. */

void fs_init(void);

int fb_init(void);
void my_fb_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

int mouse_init(void);
int keyboard_init(void);

void create_ui(void);
void *tick_routine (void *args);

/* File system driver functions. */

bool fs_ready_cb(struct _lv_fs_drv_t *drv);
void *fs_open_cb(struct _lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode);
lv_fs_res_t fs_close_cb(struct _lv_fs_drv_t *drv, void *file_p);
lv_fs_res_t fs_read_cb(struct _lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br);
lv_fs_res_t fs_seek_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence);
lv_fs_res_t fs_tell_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p);

/* Mouse driver-related structures. */

#define IOCTL_MOUSE_GET_STATE 0
#define IOCTL_MOUSE_SET_RES   1

struct ps2_mouse {
	uint16_t x, y;
	uint8_t left, right, middle;
};

struct display_res {
	uint16_t h, v;
};

/* Keyboard driver-related structures. */

#define IOCTL_KB_GET_KEY 0

struct ps2_key {
	/* UTF-8 char */
	uint32_t value;

	/*
	 * 1st bit -> whether key was pressed or released
	 * 2nd bit -> whether the shift modifier was activated or not
	 * 3rd bit -> indicates if the first scan codes was 0xE0
	 */
	uint8_t state;
};
