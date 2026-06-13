/*
 * Copyright (C) 2025 André Costa <andre_miguel_costa@hotmail.com>
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

#include <stdio.h>

#include <lvgl.h>

static bool fs_ready_cb(struct _lv_fs_drv_t *drv);
static void *fs_open_cb(struct _lv_fs_drv_t *drv, const char *path,
			lv_fs_mode_t mode);
static lv_fs_res_t fs_close_cb(struct _lv_fs_drv_t *drv, void *file_p);

static lv_fs_res_t fs_read_cb(struct _lv_fs_drv_t *drv, void *file_p, void *buf,
			      uint32_t btr, uint32_t *br);

static lv_fs_res_t fs_seek_cb(struct _lv_fs_drv_t *drv, void *file_p,
			      uint32_t pos, lv_fs_whence_t whence);

static lv_fs_res_t fs_tell_cb(struct _lv_fs_drv_t *drv, void *file_p,
			      uint32_t *pos_p);

int slv_fs_init(void)
{
	static lv_fs_drv_t drv;
	lv_fs_drv_init(&drv);

	drv.letter = 'S'; /* An uppercase letter to identify the drive */
	drv.ready_cb =
		fs_ready_cb; /* Callback to tell if the drive is ready to use */

	drv.open_cb = fs_open_cb; /* Callback to open a file */
	drv.close_cb = fs_close_cb; /* Callback to close a file */
	drv.read_cb = fs_read_cb;
	drv.seek_cb = fs_seek_cb; /* Callback to seek in a file (Move cursor) */
	drv.tell_cb = fs_tell_cb; /* Callback to tell the cursor position */

	lv_fs_drv_register(&drv);
	return 0;
}

void slv_fs_terminate(void)
{
}

static bool fs_ready_cb(struct _lv_fs_drv_t *drv)
{
	return true;
}

static void *fs_open_cb(struct _lv_fs_drv_t *drv, const char *path,
			lv_fs_mode_t mode)
{
	FILE *fp = fopen(path, (mode & LV_FS_MODE_WR) ? "w" : "r");
	if (!fp) {
		return NULL;
	}

	return fp;
}

static lv_fs_res_t fs_close_cb(struct _lv_fs_drv_t *drv, void *file_p)
{
	if (0 != fclose(file_p)) {
		return LV_FS_RES_UNKNOWN;
	}

	return LV_FS_RES_OK;
}

static lv_fs_res_t fs_read_cb(struct _lv_fs_drv_t *drv, void *file_p, void *buf,
			      uint32_t btr, uint32_t *br)
{
	*br = fread(buf, sizeof(uint8_t), btr, file_p);
	return LV_FS_RES_OK;
}

static lv_fs_res_t fs_seek_cb(struct _lv_fs_drv_t *drv, void *file_p,
			      uint32_t pos, lv_fs_whence_t whence)
{
	if (0 != fseek(file_p, pos, SEEK_SET)) {
		return LV_FS_RES_UNKNOWN;
	}

	return LV_FS_RES_OK;
}

static lv_fs_res_t fs_tell_cb(struct _lv_fs_drv_t *drv, void *file_p,
			      uint32_t *pos_p)
{
	*pos_p = ftell(file_p);
	return LV_FS_RES_OK;
}
