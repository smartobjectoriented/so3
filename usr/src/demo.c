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
 * demo.c
 *
 * A more complete LittlevGL demo.
 *
 * Base on:
 *  - https://github.com/littlevgl/lv_examples/blob/master/lv_tests/lv_test_theme/lv_test_theme_1.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <syscall.h>
#include <bits/ioctl_fix.h>

#include "lvgl/lvgl.h"

/* Framebuffer constants. */
#define BUF_SIZE (LV_HOR_RES_MAX * LV_VER_RES_MAX)
#define FB_SIZE (BUF_SIZE * LV_COLOR_DEPTH / 8)

/* Pointer on the framebuffer. */
static uint32_t *fbp;

/* File descriptor of the mouse and keyboard input device. */
static int mfd;
static int kfd;

/* Group for the keyboard. */
static lv_group_t *keyboard_group;


/* Function prototypes. */

void fs_init(void);
int fb_init(void);
void my_fb_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

int mouse_init(void);
bool my_mouse_cb(lv_indev_drv_t *indev, lv_indev_data_t *data);

int keyboard_init(void);
bool my_keyboard_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

void create_ui(void);
void *tick_routine (void *args);

/* File system driver functions. */
bool fs_ready_cb(struct _lv_fs_drv_t *drv);
lv_fs_res_t fs_open_cb(struct _lv_fs_drv_t *drv, void *file_p, const char *path, lv_fs_mode_t mode);
lv_fs_res_t fs_close_cb(struct _lv_fs_drv_t *drv, void *file_p);
lv_fs_res_t fs_read_cb(struct _lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br);
lv_fs_res_t fs_seek_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t pos);
lv_fs_res_t fs_tell_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p);

/* Mouse driver-related structures. */
#define GET_STATE 0
#define SET_SIZE  1

struct ps2_mouse {
	uint16_t x, y;
	uint8_t left, right, middle;
};

struct display_res {
	uint16_t h, v;
};

/* Keyboard driver-related structures. */
#define GET_KEY 0

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

/* Main code. */
int main(int argc, char **argv)
{
	/* Initialisation of lvgl. */
	lv_init();
	fs_init();

	/* Initialisation of the framebuffer. */
	if (fb_init()) {
		return -1;
	}

	/* lvgl will draw the screen into this buffer. */
	static lv_color_t buf[BUF_SIZE];
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
	disp_drv.flush_cb = my_fb_cb;
	lv_disp_drv_register(&disp_drv);

	/* Initialisation of the mouse and keyboard. */
	if (mouse_init() || keyboard_init()) {
		return -1;
	}

	/* Creating the UI. */
	create_ui();

	/* LittlevGL needs to know how time passes by. */
	pthread_t tick_thread;
	if(-1 == pthread_create(&tick_thread, NULL, tick_routine, NULL)) {
		return -1;
	}

	/* LittlevGL has a set of tasks it needs to run every 5ms approx. */
	while (1) {
		lv_task_handler();
		usleep(5000);
	}

	return 0;
}

/* Tick routine for lvgl. */
void *tick_routine (void *args)
{
	while(1) {
		/* Tell LittlevGL that 5 milliseconds were elapsed */
		usleep(5000);
		lv_tick_inc(5);
	}
}

/* File system driver initialisation. */
void fs_init(void)
{
	lv_fs_drv_t drv;
	lv_fs_drv_init(&drv);

	drv.letter = 'S';			/* An uppercase letter to identify the drive */
	drv.file_size = sizeof(FILE*);		/* Size required to store a file object */
	drv.rddir_size = sizeof(FILE*);		/* Size required to store a directory object (used by dir_open/close/read) */
	drv.ready_cb = fs_ready_cb;		/* Callback to tell if the drive is ready to use */

	drv.open_cb = fs_open_cb;		/* Callback to open a file */
	drv.close_cb = fs_close_cb;		/* Callback to close a file */
	drv.read_cb = fs_read_cb;		/* Callback to read a file */
	drv.seek_cb = fs_seek_cb;		/* Callback to seek in a file (Move cursor) */
	drv.tell_cb = fs_tell_cb;		/* Callback to tell the cursor position */

	drv.write_cb = NULL;			/* Callback to write a file */
	drv.trunc_cb = NULL;			/* Callback to delete a file */
	drv.size_cb = NULL;			/* Callback to tell a file's size */
	drv.rename_cb = NULL;			/* Callback to rename a file */
	drv.dir_open_cb = NULL;			/* Callback to open directory to read its content */
	drv.dir_read_cb = NULL;			/* Callback to read a directory's content */
	drv.dir_close_cb = NULL;		/* Callback to close a directory */
	drv.free_space_cb = NULL;		/* Callback to tell free space on the drive */

	lv_fs_drv_register(&drv);
}

bool fs_ready_cb(struct _lv_fs_drv_t *drv)
{
	return true;
}

lv_fs_res_t fs_open_cb(struct _lv_fs_drv_t *drv, void *file_p, const char *path, lv_fs_mode_t mode)
{
	FILE *fp;
	int flags;

	flags = mode & LV_FS_MODE_WR ? O_WRONLY : 0;
	flags |= mode & LV_FS_MODE_RD ? O_RDONLY : 0;

	fp = fopen(path, "r");
	if (!fp) {
		return LV_FS_RES_UNKNOWN;
	}

	*((FILE **)file_p) = fp;
	return LV_FS_RES_OK;
}

lv_fs_res_t fs_close_cb(struct _lv_fs_drv_t *drv, void *file_p)
{
	if (0 != fclose(*(FILE **)file_p)) {
		return LV_FS_RES_UNKNOWN;
	}

	return LV_FS_RES_OK;
}

lv_fs_res_t fs_read_cb(struct _lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
	*br = fread(buf, sizeof(uint8_t), btr, *(FILE **)file_p);
	return LV_FS_RES_OK;
}

lv_fs_res_t fs_seek_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t pos)
{
	if (0 != fseek(*(FILE **)file_p, pos, SEEK_SET)) {
		return LV_FS_RES_UNKNOWN;
	}

	return LV_FS_RES_OK;
}

lv_fs_res_t fs_tell_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
	*pos_p = ftell(*(FILE **)file_p);
	return LV_FS_RES_OK;
}


/*
 * Framebuffer initialisation.
 */
int fb_init(void)
{
	int fd;

	/* Get file descriptor. */
	fd = open("/dev/fb0", 0);
	if (-1 == fd) {
		printf("Couldn't open framebuffer.\n");
		return -1;
	}

	/* Map the fb into process memory. */
	fbp = sys_mmap(FB_SIZE, 0, fd, 0);
	if (!fbp) {
		printf("Couldn't map framebuffer.\n");
		return -1;
	}

	return 0;
}

/* Framebuffer callback. TODO copy whole region? */
void my_fb_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
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

/*
 * Mouse initialisation.
 */
int mouse_init(void)
{
	mfd = open("/dev/input0", 0);
	if (-1 == mfd) {
		printf("Couldn't open input device.\n");
		return -1;
	}

	/* We need to place res on the heap so the kernel can read it. */
	struct display_res *res = malloc(sizeof(struct display_res));
	if (!res) {
		printf("Insufficient memory.\n");
		return -1;
	}

	res->h = LV_HOR_RES_MAX;
	res->v = LV_VER_RES_MAX;

	/*
	 * Informing the driver of the screen size so it knows the bounds for
	 * the cursor coordinates.
	 */
	ioctl(mfd, SET_SIZE, res);
	free(res);

	/*
	 * Initialisation of an input driver, in our case for a mouse. Also set
	 * a cursor image. We also set a callback function which will be called
	 * by lvgl periodically. This function queries the mouse driver for the
	 * xy coordinates and button states.
	 */
	lv_indev_drv_t mouse_drv;
	lv_indev_drv_init(&mouse_drv);
	mouse_drv.type = LV_INDEV_TYPE_POINTER;
	mouse_drv.read_cb = my_mouse_cb;

	lv_indev_t *mouse_dev = lv_indev_drv_register(&mouse_drv);
	lv_obj_t *cursor_obj =  lv_img_create(lv_disp_get_scr_act(NULL), NULL);
	lv_img_set_src(cursor_obj, LV_SYMBOL_PLUS);
	lv_indev_set_cursor(mouse_dev, cursor_obj);

	return 0;
}

/* Mouse callback. */
bool my_mouse_cb(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
	/* Initialise once and place it in the heap. Not freed. */
	static struct ps2_mouse *mouse = NULL;
	if (!mouse) {
		mouse = malloc(sizeof(struct ps2_mouse));
		if (!mouse) {
			printf("Insufficient memory.\n");
			exit(EXIT_FAILURE);
		}
	}

	/* Retrieve mouse state from the driver. */
	ioctl(mfd, GET_STATE, mouse);

	data->state = mouse->left ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
	data->point.x = mouse->x;
	data->point.y = mouse->y;

	return false;
}

/*
 * Keyboard initialisation.
 */
int keyboard_init(void)
{
	kfd = open("/dev/input1", 0);
	if (-1 == kfd) {
		printf("Couldn't open input device.\n");
		return -1;
	}

	/**
	 * Initialisation of the keyboard input driver.
	 */
	lv_indev_drv_t keyboard_drv;
	lv_indev_drv_init(&keyboard_drv);
	keyboard_drv.type = LV_INDEV_TYPE_KEYPAD;
	keyboard_drv.read_cb = my_keyboard_cb;

	lv_indev_t *keyboard_dev = lv_indev_drv_register(&keyboard_drv);

	keyboard_group = lv_group_create();
	lv_indev_set_group(keyboard_dev, keyboard_group);

	return 0;
}

/* Keyboard callback. */
bool my_keyboard_cb(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
	/* Initialise once and place it in the heap. Not freed. */
	static struct ps2_key *key = NULL;
	if (!key) {
		key = malloc(sizeof(struct ps2_key));
		if (!key) {
			printf("Insufficient memory.\n");
			exit(EXIT_FAILURE);
		}
	}

	/* Retrieve mouse state from the driver. */
	ioctl(kfd, GET_KEY, key);

	if (key->value != 0) {
		data->key = key->value;
		data->state = key->state & 1;
	}

	return false;
}

/*
 * UI creation.
 */
void create_tab1(lv_obj_t *parent)
{
	lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY);

	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = LV_DPI / 10;
	h_style.body.padding.left = LV_DPI / 4;
	h_style.body.padding.right = LV_DPI / 4;
	h_style.body.padding.top = LV_DPI / 10;
	h_style.body.padding.bottom = LV_DPI / 10;

	lv_obj_t *h = lv_cont_create(parent, NULL);
	lv_obj_set_style(h, &h_style);
	lv_obj_set_click(h, false);
	lv_cont_set_fit(h, LV_FIT_TIGHT);
	lv_cont_set_layout(h, LV_LAYOUT_COL_M);

	lv_obj_t *btn = lv_btn_create(h, NULL);
	lv_btn_set_fit(btn, LV_FIT_TIGHT);
	lv_btn_set_toggle(btn, false);
	lv_obj_t *btn_label = lv_label_create(btn, NULL);
	lv_label_set_text(btn_label, "Button");

	lv_obj_t *img = lv_img_create(h, NULL);
	lv_img_set_src(img, "S:/reds.bin");

	btn = lv_btn_create(h, btn);
	lv_btn_set_toggle(btn, true);
	lv_btn_toggle(btn);
	btn_label = lv_label_create(btn, NULL);
	lv_label_set_text(btn_label, "Toggled");

	btn = lv_btn_create(h, btn);
	lv_btn_set_state(btn, LV_BTN_STATE_INA);
	btn_label = lv_label_create(btn, NULL);
	lv_label_set_text(btn_label, "Inactive");

	lv_obj_t *label = lv_label_create(h, NULL);
	lv_label_set_text(label, "Primary");

	label = lv_label_create(h, NULL);
	lv_label_set_text(label, "Secondary");

	label = lv_label_create(h, NULL);
	lv_label_set_text(label, "Hint");

	static const char *btnm_str[] = {"1", "2", "3", LV_SYMBOL_OK, LV_SYMBOL_CLOSE, ""};
	lv_obj_t *btnm = lv_btnm_create(h, NULL);
	lv_obj_set_size(btnm, lv_disp_get_hor_res(NULL) / 4, 2 *LV_DPI / 3);
	lv_btnm_set_map(btnm, btnm_str);
	lv_btnm_set_btn_ctrl_all(btnm, LV_BTNM_CTRL_TGL_ENABLE);
	lv_btnm_set_one_toggle(btnm, true);

	lv_obj_t *table = lv_table_create(h, NULL);
	lv_table_set_col_cnt(table, 3);
	lv_table_set_row_cnt(table, 4);
	lv_table_set_col_width(table, 0, LV_DPI / 3);
	lv_table_set_col_width(table, 1, LV_DPI / 2);
	lv_table_set_col_width(table, 2, LV_DPI / 2);
	lv_table_set_cell_merge_right(table, 0, 0, true);
	lv_table_set_cell_merge_right(table, 0, 1, true);

	lv_table_set_cell_value(table, 0, 0, "Table");
	lv_table_set_cell_align(table, 0, 0, LV_LABEL_ALIGN_CENTER);

	lv_table_set_cell_value(table, 1, 0, "1");
	lv_table_set_cell_value(table, 1, 1, "13");
	lv_table_set_cell_align(table, 1, 1, LV_LABEL_ALIGN_RIGHT);
	lv_table_set_cell_value(table, 1, 2, "ms");

	lv_table_set_cell_value(table, 2, 0, "2");
	lv_table_set_cell_value(table, 2, 1, "46");
	lv_table_set_cell_align(table, 2, 1, LV_LABEL_ALIGN_RIGHT);
	lv_table_set_cell_value(table, 2, 2, "ms");

	lv_table_set_cell_value(table, 3, 0, "3");
	lv_table_set_cell_value(table, 3, 1, "61");
	lv_table_set_cell_align(table, 3, 1, LV_LABEL_ALIGN_RIGHT);
	lv_table_set_cell_value(table, 3, 2, "ms");

	h = lv_cont_create(parent, h);

	lv_obj_t *sw_h = lv_cont_create(h, NULL);
	lv_cont_set_style(sw_h, LV_CONT_STYLE_MAIN, &lv_style_transp);
	lv_cont_set_fit2(sw_h, LV_FIT_NONE, LV_FIT_TIGHT);
	lv_obj_set_width(sw_h, LV_HOR_RES / 4);
	lv_cont_set_layout(sw_h, LV_LAYOUT_PRETTY);

	lv_obj_t *sw = lv_sw_create(sw_h, NULL);
	lv_sw_set_anim_time(sw, 250);

	sw = lv_sw_create(sw_h, sw);
	lv_sw_on(sw, LV_ANIM_OFF);

	lv_obj_t *bar = lv_bar_create(h, NULL);
	lv_bar_set_value(bar, 70, false);

	lv_obj_t *slider = lv_slider_create(h, NULL);
	lv_bar_set_value(slider, 70, false);

	lv_obj_t *line = lv_line_create(h, NULL);
	static lv_point_t line_p[2];
	line_p[0].x = 0;
	line_p[0].y = 0;
	line_p[1].x = lv_disp_get_hor_res(NULL) / 5;
	line_p[1].y = 0;

	lv_line_set_points(line, line_p, 2);

	lv_obj_t *cb = lv_cb_create(h, NULL);

	cb = lv_cb_create(h, cb);
	lv_btn_set_state(cb, LV_BTN_STATE_TGL_REL);

	lv_obj_t *ddlist = lv_ddlist_create(h, NULL);
	lv_ddlist_set_fix_width(ddlist, lv_obj_get_width(ddlist) + LV_DPI / 2);   /*Make space for the arrow*/
	lv_ddlist_set_draw_arrow(ddlist, true);

	h = lv_cont_create(parent, h);

	lv_obj_t *list = lv_list_create(h, NULL);
	lv_obj_set_size(list, lv_disp_get_hor_res(NULL) / 4, lv_disp_get_ver_res(NULL) / 2);
	lv_obj_t *list_btn;
	list_btn = lv_list_add_btn(list, LV_SYMBOL_GPS,  "GPS");
	lv_btn_set_toggle(list_btn, true);

	lv_list_add_btn(list, LV_SYMBOL_WIFI, "WiFi");
	lv_list_add_btn(list, LV_SYMBOL_GPS, "GPS");
	lv_list_add_btn(list, LV_SYMBOL_AUDIO, "Audio");
	lv_list_add_btn(list, LV_SYMBOL_VIDEO, "Video");
	lv_list_add_btn(list, LV_SYMBOL_CALL, "Call");
	lv_list_add_btn(list, LV_SYMBOL_BELL, "Bell");
	lv_list_add_btn(list, LV_SYMBOL_FILE, "File");
	lv_list_add_btn(list, LV_SYMBOL_EDIT, "Edit");
	lv_list_add_btn(list, LV_SYMBOL_CUT,  "Cut");
	lv_list_add_btn(list, LV_SYMBOL_COPY, "Copy");

	lv_obj_t *roller = lv_roller_create(h, NULL);
	lv_roller_set_options(roller, "Monday\nTuesday\nWednesday\nThursday\nFriday\nSaturday\nSunday", true);
	lv_roller_set_selected(roller, 1, false);
	lv_roller_set_visible_row_count(roller, 3);
}

void create_tab2(lv_obj_t *parent)
{
	lv_coord_t w = lv_page_get_scrl_width(parent);

	lv_obj_t *chart = lv_chart_create(parent, NULL);
	lv_chart_set_type(chart, LV_CHART_TYPE_AREA);
	lv_obj_set_size(chart, w / 3, lv_disp_get_ver_res(NULL) / 3);
	lv_obj_set_pos(chart, LV_DPI / 10, LV_DPI / 10);
	lv_chart_series_t *s1 = lv_chart_add_series(chart, LV_COLOR_RED);
	lv_chart_set_next(chart, s1, 30);
	lv_chart_set_next(chart, s1, 20);
	lv_chart_set_next(chart, s1, 10);
	lv_chart_set_next(chart, s1, 12);
	lv_chart_set_next(chart, s1, 20);
	lv_chart_set_next(chart, s1, 27);
	lv_chart_set_next(chart, s1, 35);
	lv_chart_set_next(chart, s1, 55);
	lv_chart_set_next(chart, s1, 70);
	lv_chart_set_next(chart, s1, 75);

	lv_obj_t *gauge = lv_gauge_create(parent, NULL);
	lv_gauge_set_value(gauge, 0, 40);
	lv_obj_set_size(gauge, w / 4, w / 4);
	lv_obj_align(gauge, chart, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 4);

	lv_obj_t *arc = lv_arc_create(parent, NULL);
	lv_obj_align(arc, gauge, LV_ALIGN_OUT_BOTTOM_MID, 0, LV_DPI / 8);

	lv_obj_t *ta = lv_ta_create(parent, NULL);
	lv_obj_set_size(ta, w / 3, lv_disp_get_ver_res(NULL) / 4);
	lv_obj_align(ta, NULL, LV_ALIGN_IN_TOP_RIGHT, -LV_DPI / 10, LV_DPI / 10);
	lv_ta_set_cursor_type(ta, LV_CURSOR_BLOCK);
	lv_ta_set_placeholder_text(ta, "Write your text here…");
	lv_group_add_obj(keyboard_group, ta);

	lv_obj_t *kb = lv_kb_create(parent, NULL);
	lv_obj_set_size(kb, 2 *w / 3, lv_disp_get_ver_res(NULL) / 3);
	lv_obj_align(kb, ta, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, LV_DPI);
	lv_kb_set_ta(kb, ta);
	lv_group_add_obj(keyboard_group, kb);

	lv_obj_t *loader = lv_preload_create(parent, NULL);
	lv_obj_align(loader, NULL, LV_ALIGN_CENTER, 0, - LV_DPI);
}

void create_tab3(lv_obj_t *parent)
{
	/*Create a Window*/
	lv_obj_t *win = lv_win_create(parent, NULL);
	lv_obj_t *win_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE);
	lv_obj_set_event_cb(win_btn, lv_win_close_event_cb);
	lv_win_add_btn(win, LV_SYMBOL_DOWN);
	lv_obj_set_size(win, lv_disp_get_hor_res(NULL) / 2, lv_disp_get_ver_res(NULL) / 2);
	lv_obj_set_pos(win, LV_DPI / 20, LV_DPI / 20);
	lv_obj_set_top(win, true);


	/*Create a Label in the Window*/
	lv_obj_t *label = lv_label_create(win, NULL);
	lv_label_set_text(label, "Label in the window");

	/*Create a  Line meter in the Window*/
	lv_obj_t *lmeter = lv_lmeter_create(win, NULL);
	lv_obj_align(lmeter, label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_lmeter_set_value(lmeter, 70);

	/*Create a 2 LEDs in the Window*/
	lv_obj_t *led1 = lv_led_create(win, NULL);
	lv_obj_align(led1, lmeter, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 2, 0);
	lv_led_on(led1);

	lv_obj_t *led2 = lv_led_create(win, NULL);
	lv_obj_align(led2, led1, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 2, 0);
	lv_led_off(led2);

	/*Create a Page*/
	lv_obj_t *page = lv_page_create(parent, NULL);
	lv_obj_set_size(page, lv_disp_get_hor_res(NULL) / 3, lv_disp_get_ver_res(NULL) / 2);
	lv_obj_set_top(page, true);
	lv_obj_align(page, win, LV_ALIGN_IN_TOP_RIGHT,  LV_DPI, LV_DPI);

	label = lv_label_create(page, NULL);
	lv_label_set_text(label,
			"Lorem ipsum dolor sit amet, repudiare voluptatibus pri cu.\n"
			"Ei mundi pertinax posidonium eum, cum tempor maiorum at,\n"
			"mea fuisset assentior ad. Usu cu suas civibus iudicabit.\n"
			"Eum eu congue tempor facilisi. Tale hinc unum te vim.\n"
			"Te cum populo animal eruditi, labitur inciderint at nec.\n\n"
			"Eius corpora et quo. Everti voluptaria instructior est id,\n"
			"vel in falli primis. Mea ei porro essent admodum,\n"
			"his ei malis quodsi, te quis aeterno his.\n"
			"Qui tritani recusabo reprehendunt ne,\n"
			"per duis explicari at. Simul mediocritatem mei et.");

	/*Create a Calendar*/
	lv_obj_t *cal = lv_calendar_create(parent, NULL);
	lv_obj_set_size(cal, 5 * LV_DPI / 2, 5 * LV_DPI / 2);
	lv_obj_align(cal, page, LV_ALIGN_OUT_RIGHT_TOP, -LV_DPI / 2, LV_DPI / 3);
	lv_obj_set_top(cal, true);

	static lv_calendar_date_t highlighted_days[2];
	highlighted_days[0].day = 5;
	highlighted_days[0].month = 5;
	highlighted_days[0].year = 2018;

	highlighted_days[1].day = 8;
	highlighted_days[1].month = 5;
	highlighted_days[1].year = 2018;

	lv_calendar_set_highlighted_dates(cal, highlighted_days, 2);
	lv_calendar_set_today_date(cal, &highlighted_days[0]);
	lv_calendar_set_showed_date(cal, &highlighted_days[0]);

	/*Create a Message box*/
	static const char *mbox_btn_map[] = {" ", "Got it!", " ", ""};
	lv_obj_t *mbox = lv_mbox_create(parent, NULL);
	lv_mbox_set_text(mbox, "Click on the window or the page to bring it to the foreground");
	lv_mbox_add_btns(mbox, mbox_btn_map);
	lv_btnm_set_btn_ctrl(lv_mbox_get_btnm(mbox), 0, LV_BTNM_CTRL_HIDDEN);
	lv_btnm_set_btn_width(lv_mbox_get_btnm(mbox), 1, 7);
	lv_btnm_set_btn_ctrl(lv_mbox_get_btnm(mbox), 2, LV_BTNM_CTRL_HIDDEN);
	lv_obj_set_top(mbox, true);
}

void create_ui()
{
	lv_obj_t *scr = lv_cont_create(NULL, NULL);
	lv_disp_load_scr(scr);

	lv_obj_t *tv = lv_tabview_create(scr, NULL);
	lv_obj_set_size(tv, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
	lv_obj_t *tab1 = lv_tabview_add_tab(tv, "Tab 1");
	lv_obj_t *tab2 = lv_tabview_add_tab(tv, "Tab 2");
	lv_obj_t *tab3 = lv_tabview_add_tab(tv, "Tab 3");

	create_tab1(tab1);
	create_tab2(tab2);
	create_tab3(tab3);
}
