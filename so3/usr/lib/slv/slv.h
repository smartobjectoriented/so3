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

/*
 * So3-LVgl bridge (SLV)
 * 
 * SLV adds the necessary HAL to run LVGL apps 
 *
 */

#ifndef SLV_H
#define SLV_H

#include <stdbool.h>
#include <pthread.h>

#include "slv_fb.h"

typedef struct {
	int mfd, kfd;
	slv_fb_t fb;
	pthread_t tick_thread, loop_thread;
	bool has_loop_thread;
	bool terminate;
} slv_t;

/*
 * Inits LVGL and necessary drivers
 * Returns 0 on success or negative value on error.
 */
int slv_init(slv_t *slv);

/*
 * Loops forever
 */
void slv_loop(slv_t *slv);

/*
 * Start the Main Loop in a different thread.
 * Returns 0 on success or negative value on error.
 */
int slv_loop_start(slv_t *slv);

/*
 * Terminates SLV and LVGL
 */
void slv_terminate(slv_t *data);

#endif
