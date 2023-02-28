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

#if 1
#define DEBUG
#endif

#include <thread.h>
#include <string.h>

#include <soo/xmlui.h>

#include <soo/dev/vuihandler.h>
#include <soo/dev/vwagoled.h>
#include <me/wagoled.h>

#define LED_PER_ROOM	6



void wagoled_process_event(char *data, size_t size) {
	char id[ID_MAX_LENGTH];
	char action[ACTION_MAX_LENGTH];

	static switch_status sw_status_r = STATUS_OFF; 
	static switch_status sw_status_l = STATUS_OFF; 

	memset(id, 0, ID_MAX_LENGTH);
	memset(action, 0, ACTION_MAX_LENGTH);

	xml_parse_event(data, id, action);

	if (!strcmp(action, "clickDown")) {

		if (!strcmp(id, BUTTON_LED_R_ID)) {
			sh_wagoled->sw_pos = POS_RIGHT_UP;
			sw_status_r = sw_status_r == STATUS_OFF ? STATUS_ON : STATUS_OFF;
			sh_wagoled->sw_status = sw_status_r;

		} else if(!strcmp(id, BUTTON_LED_L_ID)) {
			sh_wagoled->sw_pos = POS_LEFT_UP;
			sw_status_l = sw_status_l == STATUS_OFF ? STATUS_ON : STATUS_OFF;
			sh_wagoled->sw_status = sw_status_l;
			
		}
		complete(&send_data_lock);
	}


}

void wagoled_send_model(void) {
	vuihandler_send(WAGOLED_MODEL, strlen(WAGOLED_MODEL)+1, VUIHANDLER_SELECT);
}

void *wagoled_send_cmd(void *args) {
	int room1_ids [] = {1, 2, 3, 4, 5, 6};
	int room2_ids [] = {7, 8, 9, 10, 11, 12};
	int selected_room[LED_PER_ROOM] = {0};
	wago_cmd_t room_cmd = LED_OFF;

	vwagoled_set(room1_ids, LED_PER_ROOM, LED_OFF);
	vwagoled_set(room2_ids, LED_PER_ROOM, LED_OFF);

	while(atomic_read(&shutdown)) {
		wait_for_completion(&send_data_lock);

		switch(sh_wagoled->sw_pos) {
			case POS_LEFT_UP:
				memcpy(selected_room, room1_ids, sizeof(selected_room));
				break;
			case POS_RIGHT_UP:
				memcpy(selected_room, room2_ids, sizeof(selected_room));
				break;
			default:
				DBG("switch postion %d not supported\n", sh_wagoled->sw_pos);
				continue;
		}

		switch(sh_wagoled->sw_status) {
			case STATUS_OFF:
				room_cmd = LED_OFF;
				break;
			case STATUS_ON:
				room_cmd = LED_ON;
				break;
			default:
				DBG("switch status %d not supported\n", sh_wagoled->sw_status);
				continue;

		}
		
		DBG("Set LED %s\n", room_cmd == LED_ON ? "On" : "Off");
		vwagoled_set(selected_room, LED_PER_ROOM, room_cmd);
	}
	return 0;
}

void *app_thread_main(void *args) {
	tcb_t *wagoled_th;

	/* The ME can cooperate with the others. */
	spad_enable_cooperate();
	printk("Welcome to WAGO led ME\n");

	vuihandler_register_callbacks(wagoled_send_model, wagoled_process_event);

	wagoled_th = kernel_thread(wagoled_send_cmd, "wagoled_send_command", NULL, THREAD_PRIO_DEFAULT);
	thread_join(wagoled_th);

	return 0;
}
