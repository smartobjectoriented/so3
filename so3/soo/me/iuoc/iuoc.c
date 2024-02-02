/*
 * Copyright (C) 2023 A.Gabriel Catel Torres <arzur.cateltorres@heig-vd.ch>
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
 * Description: This file is the implementation of the IUOC ME. This code is 
 * responsible of managing the data incoming from any ME and from the IUOC server
 * that are allowed to communicate with the IUOC.
 */

#if 1
#define DEBUG
#endif

#include <thread.h>
#include <string.h>

#include <soo/xmlui.h>

#include <soo/dev/viuoc.h>
#include <delay.h>

#include <me/iuoc.h>
#include <me/switch.h>

/**
 * @brief Thread to acquire iuoc events
 * 
 * @param args not used for now
 */
void *iuoc_wait_data_th(void *args) {

	iuoc_data_t iuoc_data;
	int ret, i;
	bool local_coop = false; 

	DBG("[IUOC ME] ME thread receiver set up !\n");

	while (1) {
		ret = get_iuoc_me_data(&iuoc_data);

		if (ret) {
			continue;
		}

		local_coop = true;

		/* The data recieved will fill different structures to fit the ME cooperation */
		switch (iuoc_data.me_type) {

		case IUOC_ME_BLIND:
			sh_blind->timestamp = iuoc_data.timestamp;

			for (i = 0; i < iuoc_data.data_array_size; i++) {

				if (!strcmp(iuoc_data.data_array[i].name, "direction")) {
					sh_blind->direction = iuoc_data.data_array[i].value;
					DBG("[IUOC ME] Blind direction recieved = %d\n", sh_blind->direction);
				} 
				
				else if (!strcmp(iuoc_data.data_array[i].name, "action_mode")) {
					sh_blind->action_mode = iuoc_data.data_array[i].value;
					DBG("[IUOC ME] Blind action recieved = %d\n", sh_blind->action_mode);
				}
			}
			
			break;
			
		default:
			printk("[IUOC ME] Data coming from unsupported ME...\n");
			local_coop = false;
			break;
		}

		if (local_coop) {
			do_local_cooperation(ME_domID());
		}
	}
}

void *app_thread_main(void *args) {
	tcb_t *iuoc_recv_th;

	/* The ME can cooperate with the others. */
	spad_enable_cooperate();

	DBG("Welcome to IUOC ME\n");

	iuoc_recv_th = kernel_thread(iuoc_wait_data_th, "iuoc_wait_data", NULL, THREAD_PRIO_DEFAULT);

	thread_join(iuoc_recv_th);

	return 0;
}
