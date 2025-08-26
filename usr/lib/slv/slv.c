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

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <bits/ioctl.h>
#include <sys/mman.h>
#include <time.h>

#include <lvgl.h>

#include "slv.h"
#include "slv_fb.h"
#include "slv_keyboard.h"
#include "slv_fs.h"
#include "slv_mouse.h"

static uint32_t slv_tick_get(void);
static void *slv_loop_inner(void *args);

int slv_init(slv_t *slv)
{
	lv_init();
	lv_tick_set_cb(slv_tick_get);
	slv_fs_init();
	int err = slv_fb_init(&slv->fb);
	if (err < 0) {
		goto fb_err;
	}
	slv->kfd = slv_keyboard_init();
	if (slv->kfd < 0) {
		err = slv->kfd;
		goto kb_err;
	}
	slv->mfd = slv_mouse_init(slv->fb.hres, slv->fb.vres);
	if (slv->mfd < 0) {
		err = slv->mfd;
		goto mouse_err;
	}
	slv->terminate = false;

	return 0;

fb_err:
	lv_deinit();
kb_err:
	slv_fb_terminate(&slv->fb);
mouse_err:
	slv_keyboard_terminate(slv->kfd);
	slv_mouse_terminate(slv->mfd);
	return err;
}

void slv_loop(slv_t *slv)
{
	slv_loop_inner(slv);
}

int slv_loop_start(slv_t *slv)
{
	int err = pthread_create(&slv->loop_thread, NULL, slv_loop_inner, NULL);
	slv->has_loop_thread = err >= 0;
	return err;
}

void slv_terminate(slv_t *slv)
{
	slv->terminate = true;

	if (slv->has_loop_thread) {
		pthread_join(slv->has_loop_thread, NULL);
	}

	lv_deinit();
	slv_fs_terminate();
	slv_fb_terminate(&slv->fb);
	slv_keyboard_terminate(slv->kfd);
	slv_mouse_terminate(slv->mfd);
}
static void *slv_loop_inner(void *args)
{
	slv_t *slv = (slv_t *)args;
	while (!slv->terminate) {
		const uint32_t next_timer = lv_timer_handler();
		if (next_timer == LV_NO_TIMER_READY) {
			usleep(LV_DEF_REFR_PERIOD * 1000);
		} else {
			usleep((uint64_t)next_timer * 1000);
		}
	}
	return NULL;
}

static uint32_t slv_tick_get(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint32_t tick = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	return tick;
}
