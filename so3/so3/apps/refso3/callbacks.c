/*
 * Copyright (C) 2014-2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
 * Copyright (C) March 2018 Baptiste Delporte <bonel@bonel.net>
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

#include <memory.h>
#include <completion.h>

#include <soo/avz.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/debug.h>

#include <capsule/refso3.h>

#include <asm/mmu.h>

static LIST_HEAD(visits);
static LIST_HEAD(known_soo_list);

/* Reference to the shared content helpful during synergy with other MEs */
sh_refso3_t *sh_refso3;

/**
 * PRE_SUSPEND
 *
 * This callback is executed right before suspending the state of frontend drivers, before migrating
 *
 */
void cb_pre_suspend(soo_domcall_arg_t *args)
{
	DBG(">> capsule %d: cb_pre_suspend...\n", S3C_domID());
}

/**
 * PRE_RESUME
 *
 * This callback is executed right before resuming the frontend drivers, right after capsule activation
 *
 * Returns 0 if no propagation to the user space is required, 1 otherwise
 */
void cb_pre_resume(soo_domcall_arg_t *args)
{
	DBG(">> capsule %d: cb_pre_resume...\n", S3C_domID());
}

/**
 * POST_ACTIVATE callback (async)
 */
void cb_post_activate(soo_domcall_arg_t *args)
{
#if 0
	agency_ctl_args_t agency_ctl_args;
	static uint32_t count = 0;
#endif

	DBG(">> capsule %d: cb_post_activate...\n", S3C_domID());
}

/**
 * SHUTDOWN callback (async)
 *
 * Returns 0 if no propagation to the user space is required, 1 otherwise
 *
 */

void cb_shutdown(void)
{
	DBG(">> capsule %d: cb_shutdown...\n", S3C_domID());
	DBG("capsule state: %d\n", get_S3C_state());

	/* We do nothing particular here for this capsule,
	 * however we proceed with the normal termination of execution.
	 */

	set_S3C_state(S3C_state_terminated);
}

void callbacks_init(void)
{
	/* Allocate the shared page. */
	sh_refso3 = (sh_refso3_t *) get_contig_free_vpages(1);

	/* Initialize the shared content page used to exchange information between other MEs */
	memset(sh_refso3, 0, PAGE_SIZE);
}
