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

#include <soo/dev/vknx.h>
#include <soo/dev/vuihandler.h>
#include <soo/debug.h>
#include <soo/xmlui.h>
#include <timer.h>
#include <delay.h>

#include <me/blind.h>

#define BLIND_FIRST_DP_ID		0x01


void blind_send_model(void) {
	vuihandler_send(BLIND_MODEL, strlen(BLIND_MODEL)+1, VUIHANDLER_SELECT);
}

void blind_process_events(char *data, size_t size) {
	char id[ID_MAX_LENGTH];
	char action[ACTION_MAX_LENGTH];

	memset(id, 0, ID_MAX_LENGTH);
	memset(action, 0, ACTION_MAX_LENGTH);

	xml_parse_event(data, id, action);

	if (!strcmp(action, "clickDown")) {

		if (!strcmp(id, BTN_BLIND_UP_ID)) {
			sh_blind->direction = BLIND_UP;
			sh_blind->action_mode = BLIND_STEP;

		} else if(!strcmp(id, BTN_BLIND_DOWN_ID)) {
			sh_blind->direction = BLIND_DOWN;
			sh_blind->action_mode = BLIND_STEP;
			
		} else if(!strcmp(id, BTN_BLIND_UP_LONG_ID)) {
			sh_blind->direction = BLIND_UP;
			sh_blind->action_mode = BLIND_FULL;
			
		} else if(!strcmp(id, BTN_BLIND_DOWN_LONG_ID)) {
			sh_blind->direction = BLIND_DOWN;
			sh_blind->action_mode = BLIND_FULL;
			
		}
		complete(&send_data_lock);
	}
}

/**
 * @brief Generic blind initialization
 * 
 * @param bl Blind to init
 */
void blind_init(blind_t *bl)
{
#ifdef BLIND_VBWA88PG
	bl->type = VBWA88PG;
#endif

	switch (bl->type)
	{
	case VBWA88PG:
		vbwa88pg_blind_init(&bl->blind, BLIND_FIRST_DP_ID);
		break;
	
	default:
		break;
	}
}

/**
 * @brief Generic blind up
 * 
 * @param bl Blind to move up
 */
void blind_up(blind_t *bl) {
	DBG(MEBLIND_PREFIX "%s\n", __func__);

	switch(bl->type) {
		case VBWA88PG:
			if (sh_blind->action_mode == BLIND_STEP) {
				bl->blind.dps[INC_DEC_STOP].data[0] = VBWA88PG_BLIND_INC;

				vbwa88pg_blind_inc_dec_stop(&bl->blind);

			} else if (sh_blind->action_mode == BLIND_FULL) {
				bl->blind.dps[UP_DOWN].data[0] = VBWA88PG_BLIND_UP;

				vbwa88pg_blind_up_down(&bl->blind);
			}
			break;
		default:
			break;
	}
}

/**
 * @brief Generic blind down
 * 
 * @param bl Blind to move down
 */
void blind_down(blind_t *bl) {
	DBG(MEBLIND_PREFIX "%s\n", __func__);

	switch(bl->type) {
		case VBWA88PG:
			if (sh_blind->action_mode == BLIND_STEP) {
				bl->blind.dps[INC_DEC_STOP].data[0] = VBWA88PG_BLIND_DEC;
				vbwa88pg_blind_inc_dec_stop(&bl->blind);

			} else if (sh_blind->action_mode == BLIND_FULL) {
				bl->blind.dps[UP_DOWN].data[0] = VBWA88PG_BLIND_DOWN;
				vbwa88pg_blind_up_down(&bl->blind);
			}
			break;
		
		default:
			break;
	}
}

/**
 * @brief Thread used to send command to a blind
 * 
 * @param args (blind_t *) generic struct blind 
 * @return int 0
 */
void *blind_send_cmd_th(void *args) {
	blind_t *bl = (blind_t*)args;
	blind_init(bl);

	DBG(MEBLIND_PREFIX "Started: %s\n", __func__);

	while (atomic_read(&shutdown)) {
		
		wait_for_completion(&send_data_lock);

		switch(sh_blind->direction) {
			case BLIND_UP:
				blind_up(bl);
				break;

			case BLIND_DOWN:
				blind_down(bl);
				break;

			default:
				break;
		}	
	}

	DBG(MEBLIND_PREFIX "Stopped: %s\n", __func__);

	return NULL;
}

void *app_thread_main(void *args) {
	tcb_t *blind_th;
	blind_t *bl;

	bl = (blind_t *)malloc(sizeof(blind_t));

	/* The ME can cooperate with the others. */
	spad_enable_cooperate();

	printk(MEBLIND_PREFIX "Welcome\n");

	vuihandler_register_callbacks(blind_send_model, blind_process_events);

	blind_th = kernel_thread(blind_send_cmd_th, "blind_send_cmd_th", bl, THREAD_PRIO_DEFAULT);

	return NULL;
}
