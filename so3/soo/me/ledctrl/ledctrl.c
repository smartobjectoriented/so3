/*
 * Copyright (C) 2016-2020 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2016-2019 Baptiste Delporte <bonel@bonel.net>
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

#include <mutex.h>
#include <delay.h>
#include <timer.h>
#include <heap.h>
#include <memory.h>

#include <soo/avz.h>
#include <soo/gnttab.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/debug.h>
#include <soo/debug/dbgvar.h>
#include <soo/debug/logbool.h>
#include <soo/evtchn.h>

#include <soo/dev/vsensej.h>
#include <soo/dev/vsenseled.h>
#include <soo/dev/vuihandler.h>

#include <me/ledctrl.h>

/**
 *
 * @param args - To be compliant... Actually not used.
 * @return
 */
void *process_led(void *args) {

	while (true) {

		wait_for_completion(&upd_lock);

		if (sh_ledctrl->local_nr != sh_ledctrl->incoming_nr) {

			/* Check if we are not at the beginning, otherwise switch off first. */
			if (sh_ledctrl->local_nr != -1)
				vsenseled_set(sh_ledctrl->local_nr, 0);

			/* Switch on the correct led */
			vsenseled_set(sh_ledctrl->incoming_nr, 1);

			sh_ledctrl->local_nr = sh_ledctrl->incoming_nr;
		}
	}

	return NULL;
}

/*
 * The main application of the ME is executed right after the bootstrap. It may be empty since activities can be triggered
 * by external events based on frontend activities.
 */
void *app_thread_main(void *args) {
	struct input_event ie;

	/* The ME can cooperate with the others. */
	spad_enable_cooperate();

	printk("Enjoy the SOO.ledctrl ME !\n");

	kernel_thread(process_led, "process_led", NULL, 0);

	while (true) {
		vsensej_get(&ie);

		if (!ie.value)
			continue;

		/* Prepare to switch on the right led. */
		switch (ie.code) {

		case KEY_ENTER:
			sh_ledctrl->incoming_nr = 0;
			break;

		case KEY_LEFT:
			sh_ledctrl->incoming_nr = 1;
			break;

		case KEY_UP:
			sh_ledctrl->incoming_nr = 2;
			break;

		case KEY_RIGHT:
			sh_ledctrl->incoming_nr = 3;
			break;

		case KEY_DOWN:
			sh_ledctrl->incoming_nr = 4;
			break;

		}

		/* Retrieve the agency UID of the Smart Object on which the ME is about to be activated. */

		sh_ledctrl->initiator = sh_ledctrl->me_common.here;

		propagate();

		complete(&upd_lock);

	}

	return NULL;
}
