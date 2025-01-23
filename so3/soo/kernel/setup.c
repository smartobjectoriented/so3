/*
 * Copyright (C) 2014-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <common.h>
#include <memory.h>
#include <heap.h>
#include <initcall.h>
#include <syscall.h>
#include <banner.h>

#include <asm/cacheflush.h>
#include <asm/mmu.h>
#include <asm/setup.h>

#include <soo/hypervisor.h>
#include <soo/avz.h>
#include <soo/evtchn.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/gnttab.h>

#include <avz/uapi/avz.h>

volatile avz_shared_t *avz_shared;
 
 void resume_fn(void) {

	while (1);
 }

/**
 * This function is called at early bootstrap stage along head.S.
 */
void avz_setup(void) {

        avz_get_shared();

        avz_shared->dom_desc.u.ME.__resume_fn = resume_fn;

        lprintk("SOO Virtualizer (avz) shared page:\n\n");

	lprintk("- Dom phys offset: %lx\n\n", (addr_t) mem_info.phys_base);
	
	virq_init();
}

void post_init_setup(void) {

	printk("VBstore shared page grant reference %d\n", avz_shared->vbstore_grant_ref);

        gnttab_map(DOMID_AGENCY, avz_shared->vbstore_grant_ref, (void **) &__intf);

        printk("SOO Mobile Entity booting ...\n");

        soo_guest_activity_init();

	callbacks_init();

	/* Initialize the Vbus subsystem */
	vbus_init();

	/*
	 * Now, the ME requests to be paused by setting its state to ME_state_preparing. As a consequence,
	 * the agency will pause it.
	 * The state is moving from ME_state_booting to ME_state_preparing.
	 */
	set_ME_state(ME_state_preparing);

	/*
	 * There are two scenarios.
	 * 1. Classical injection scheme: Wait for the agency to perform the pause+unpause. It should set the ME
	 *    state to ME_state_booting to allow the ME to continue.
	 * 2. ME that has migrated on a Smart Object: The ME state is ME_state_migrating, so it is different from
	 *    ME_state_preparing.
	 */

	while (1) {
		schedule();

		if (get_ME_state() != ME_state_preparing) {
			DBG("ME state changed: %d, continuing...\n", get_ME_state());
			break;
		}
	}

	BUG_ON(get_ME_state() != ME_state_booting);

	/* Write the entries related to the ME ID in vbstore */
	vbstore_ME_ID_populate();

	/* How create all vbstore entries required by the frontend drivers */
	vbstore_init_dev_populate();

	lprintk("%s", SO3_BANNER);

	DBG("ME running as domain %d\n", ME_domID());
}

REGISTER_POSTINIT(post_init_setup)
