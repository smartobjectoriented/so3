/*
 * Copyright (C) 2016-2019 Daniel Rossier <daniel.rossier@soo.tech>
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

#include <common.h>
#include <thread.h>
#include <completion.h>
#include <heap.h>
#include <list.h>
#include <schedule.h>
#include <errno.h>

#include <device/irq.h>

#include <soo/evtchn.h>
#include <soo/vbus.h>
#include <soo/vbstore.h>
#include <soo/soo.h>
#include <soo/console.h>

/*
 * Used to keep track of the target domain for a certain (outgoing) dc_event.
 * Value -1 means no dc_event in progress.
 */
atomic_t dc_outgoing_domID[DC_EVENT_MAX];

/*
 * Used to store the domID issuing a (incoming) dc_event
 */
atomic_t dc_incoming_domID[DC_EVENT_MAX];

static struct completion dc_stable_lock[DC_EVENT_MAX];

void dc_stable(int dc_event)
{
	/* It may happen that the thread which performs the down did not have time to perform the call and is not suspended.
	 * In this case, complete() will increment the count and wait_for_completion() will go straightforward.
	 */

	complete(&dc_stable_lock[dc_event]);

	atomic_set(&dc_incoming_domID[dc_event], -1);
}

/*
 * Sends a ping event to a remote domain in order to get synchronized.
 * Various types of event (dc_event) can be sent.
 *
 * To perform a ping from the RT domain, please use rtdm_do_sync_agency() in rtdm_vbus.c
 *
 * As for the domain table, the index 0 and 1 are for the agency and the indexes 2..MAX_DOMAINS
 * are for the MEs. If a ME_slotID is provided, the proper index is given by ME_slotID.
 *
 * @domID: the target domain
 * @dc_event: type of event used in the synchronization
 */
void do_sync_dom(int domID, dc_event_t dc_event)
{
	/* Ping the remote domain to perform the task associated to the DC event */
	DBG("%s: ping domain %d...\n", __func__, domID);

	/* Make sure a previous transaction is not ongoing. */
	while (atomic_cmpxchg(&dc_outgoing_domID[dc_event], -1, domID) != -1)
		schedule();

	set_dc_event(domID, dc_event);

	DBG("%s: notifying via evtchn %d...\n", __func__,
	    avz_shared->dom_desc.u.ME.dc_evtchn);
	notify_remote_via_evtchn(avz_shared->dom_desc.u.ME.dc_evtchn);

	/* Wait for the response from the outgoing domain */
	DBG("%s: waiting for completion on dc_event %d...\n", __func__,
	    dc_event);
	wait_for_completion(&dc_stable_lock[dc_event]);

	DBG("%s: all right, got the completion, resetting the barrier.\n",
	    __func__);

	/* Now, reset the barrier. */
	atomic_set(&dc_outgoing_domID[dc_event], -1);
}

/*
 * Tell a specific domain that we are now in a stable state regarding the dc_event actions.
 * Typically, this comes after receiving a dc_event leading to a dc_event related action.
 */
void tell_dc_stable(int dc_event)
{
	int domID;

	domID = atomic_read(&dc_incoming_domID[dc_event]);

	BUG_ON(domID == -1);

	DBG("vbus_stable: now pinging domain %d back\n", domID);

	set_dc_event(domID, dc_event);

	/* Ping the remote domain to perform the task associated to the DC event */
	DBG("%s: ping domain %d...\n", __func__, dc_incoming_domID[dc_event]);

	atomic_set(&dc_incoming_domID[dc_event], -1);

	notify_remote_via_evtchn(avz_shared->dom_desc.u.ME.dc_evtchn);
}

/*
 * Prepare a remote ME to react to a ping event.
 * @domID: the target ME
 */
void set_dc_event(domid_t domID, dc_event_t dc_event)
{
	avz_hyp_t args;

	DBG("%s(%d, %d)\n", __func__, domID, dc_event);

	args.cmd = AVZ_DC_EVENT_SET;
	args.u.avz_dc_event_args.domID = domID;
	args.u.avz_dc_event_args.dc_event = dc_event;

	avz_hypercall(&args);

	while (args.u.avz_dc_event_args.state == -EBUSY) {
		schedule();

		avz_hypercall(&args);
	}
}

/*
 * Get the state of a ME.
 */
int get_ME_state(void)
{
	return avz_shared->dom_desc.u.ME.state;
}

void set_ME_state(ME_state_t state)
{
	/* Be careful if the ME is in living state and suddently is set to killed.
	 * Backends will be in a weird state.
	 */
	if ((state == ME_state_killed) &&
	    (avz_shared->dom_desc.u.ME.state == ME_state_living))
		lprintk("## WARNING ! ME %d is set to killed while living!\n",
			ME_domID());

	avz_shared->dom_desc.u.ME.state = state;
}

void postmig_setup(void);
void perform_task(dc_event_t dc_event)
{
	soo_domcall_arg_t args;

	switch (dc_event) {
	case DC_FORCE_TERMINATE:
		/* The ME will initiate the force_terminate processing on its own. */

		DBG("perform a CB_FORCE_TERMINATE on this ME %d\n", ME_domID());
		cb_force_terminate();

		if (get_ME_state() == ME_state_terminated) {
			/* Prepare vbus to stop everything with the frontend */
			/* (interactions with vbus) */
			DBG("Device shutdown...\n");
			device_shutdown();

			/* Remove all devices */
			DBG("Removing devices ...\n");
			remove_devices();

			/* Remove vbstore entries related to this ME */
			DBG("Removing vbstore entries ...\n");
			remove_vbstore_entries();
		}

		break;

	case DC_RESUME:
		DBG("resuming vbstore...\n");

		BUG_ON((get_ME_state() != ME_state_resuming) &&
		       (get_ME_state() != ME_state_awakened));

		/* Giving a chance to perform actions before resuming devices */
		args.cmd = CB_PRE_RESUME;
		do_soo_activity(&args);

		DBG("Now resuming vbstore...\n");
		vbs_resume();

		/* During a resuming after an awakened snapshot, re-init watch for device/<domID> */
		if (get_ME_state() == ME_state_awakened)
			postmig_setup();

		DBG("vbstore resumed.\n");
		break;

	case DC_SUSPEND:
		DBG("Suspending vbstore...\n");

		vbs_suspend();
		DBG("vbstore suspended.\n");

		break;

	case DC_PRE_SUSPEND:
		DBG("Pre-suspending...\n");

		/* Giving a chance to perform actions before suspending devices */
		args.cmd = CB_PRE_SUSPEND;
		do_soo_activity(&args);
		break;

	case DC_POST_ACTIVATE:
		DBG("Post_activate...\n");

		args.cmd = CB_POST_ACTIVATE;
		do_soo_activity(&args);
		break;

	default:
		lprintk("Wrong DC event %d\n", avz_shared->dc_event);
	}

	tell_dc_stable(dc_event);
}

/*
 * This function is called as a deferred work during the container kernel execution.
 */
void do_soo_activity(void *arg)
{
	soo_domcall_arg_t *args = (soo_domcall_arg_t *)arg;

	switch (args->cmd) {
	case CB_PRE_SUSPEND: /* Called by perform_pre_suspend */
		DBG("Pre-suspend callback for ME %d\n", ME_domID());

		cb_pre_suspend(arg);
		break;

	case CB_PRE_RESUME: /* Called from vbus/vbs.c */
		DBG("Pre-resume callback for ME %d\n", ME_domID());

		cb_pre_resume(arg);
		break;

	case CB_POST_ACTIVATE: /* Called by perform_post_activate() */

		DBG("Post_activate callback for ME %d\n", ME_domID());

		cb_post_activate(args);
		break;
	}
}

ME_desc_t *get_ME_desc(void)
{
	return (ME_desc_t *)&avz_shared->dom_desc.u.ME;
}

agency_desc_t *get_agency_desc(void)
{
	return (agency_desc_t *)&avz_shared->dom_desc.u.agency;
}

void soo_guest_activity_init(void)
{
	unsigned int i;

	for (i = 0; i < DC_EVENT_MAX; i++) {
		atomic_set(&dc_outgoing_domID[i], -1);
		atomic_set(&dc_incoming_domID[i], -1);

		init_completion(&dc_stable_lock[i]);
	}
}
