/*
 * Copyright (C) 2022 Mattia Gallacchi <mattia.gallaccchi@heig-vd.ch>
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

#if 0
#define DEBUG
#endif

#include <thread.h>
#include <heap.h>
#include <timer.h>

#include <soo/hypervisor.h>
#include <soo/debug.h>

#include <me/switch.h>
#include <me/common.h>

/** Temporary Switch ID for EnOcean switch **/
#define ENOCEAN_SWITCH_ID	0x002A3D45

/**
 * @brief Generic switch init
 * 
 * @param sw Switch to init
 */
void switch_init(switch_t *sw) {
#warning to be avoided...
#ifdef ENOCEAN
	ptm210_init(&sw->sw, ENOCEAN_SWITCH_ID);
#endif

#ifdef KNX
	gtl2tw_init(&sw->sw);
#endif

	sh_switch->timestamp = 0;
}

/**
 * @brief  Generic switch deinit
 * @param  sw: Switch to deinit 
 * @retval None
 */
void switch_deinit(switch_t *sw) {
#ifdef ENOCEAN
	ptm210_deinit(&sw->sw);
#endif
}
/**
 * @brief Generic switch get data. Wait for an event.
 * 
 * @param sw Switch to get data from
 */
void switch_get_data(switch_t *sw) {
#ifdef ENOCEAN

	wait_for_completion(&sw->sw._wait_event);
	if (atomic_read(&sw->sw.event)) {
		if (atomic_read(&sw->sw.up)) 
			sh_switch->pos = POS_LEFT_UP;
		else if (atomic_read(&sw->sw.down)) 
			sh_switch->pos = POS_LEFT_DOWN;
		else
			sh_switch->pos = POS_NONE;
		
		if (atomic_read(&sw->sw.released)) 
			sh_switch->press = PRESS_SHORT;
		else
			sh_switch->press = PRESS_LONG;

		sh_switch->switch_event= true;
		ptm210_reset(&sw->sw);
	} 
			
#endif
		
#ifdef KNX
	gtl2tw_wait_event(&sw->sw);

	if (sw->sw.events[POS_LEFT_UP]) {
		sh_switch->pos = POS_LEFT_UP;
		sh_switch->status = sw->sw.status[POS_LEFT_UP];
		sh_switch->switch_event = true;
	}

	if (sw->sw.events[POS_RIGHT_UP]) {
		sh_switch->pos = POS_RIGHT_UP;
		sh_switch->status = sw->sw.status[POS_RIGHT_UP];
		sh_switch->switch_event = true;
	}
	
#endif

} 

/**
 * @brief Thread to acquire switch events
 * 
 * @param args (switch_t *) generic struct switch
 * @return int 0
 */
void *switch_wait_data_th(void *args) {
	switch_t *sw = (switch_t*)args;
	switch_init(sw);

	DBG(MESWITCH_PREFIX "Started: %s\n", __func__);
	while (atomic_read(&shutdown)) {
		
		switch_get_data(sw);

		if (sh_switch->switch_event) {		
			/** migrate **/
			sh_switch->timestamp++;
#if 0
			spin_lock(&propagate_lock);
			sh_switch->need_propagate = true;
			spin_unlock(&propagate_lock);

			do_local_cooperation(ME_domID());
#endif
			DBG(MESWITCH_PREFIX "New switch event. pos: %d, press: %d, status %d\n", sh_switch->pos, sh_switch->press,
				sh_switch->status);
			sh_switch->switch_event = false;

			do_local_cooperation(ME_domID());
		} else {
			DBG(MESWITCH_PREFIX "No switch event\n");
		}
	}

	switch_deinit(sw);

	DBG(MESWITCH_PREFIX "Stopped: %s\n", __func__);

	return NULL;
}

void *app_thread_main(void *args) {
	tcb_t *switch_th;
	switch_t *sw;

	sw = (switch_t *)malloc(sizeof(switch_t));
	if (!sw) {
		DBG(MESWITCH_PREFIX "Failed to allocate switch_t\n");
		kernel_panic();
	}

	/* The ME can cooperate with the others. */
	spad_enable_cooperate();

	printk(MESWITCH_PREFIX "Welcome\n");

	switch_th = kernel_thread(switch_wait_data_th, "switch_wait_data_th", sw, THREAD_PRIO_DEFAULT);
	if (!switch_th) {
		DBG(MESWITCH_PREFIX "Failed to start switch thread\n");
		kernel_panic();
	}

	return NULL;
}
