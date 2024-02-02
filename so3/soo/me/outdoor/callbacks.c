/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@soo.tech>
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

#include <asm/mmu.h>

#include <memory.h>

#include <soo/avz.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/debug.h>

#include <me/outdoor/outdoor.h>

#include <me/eco_stability.h>

static bool outdoor_initialized = false;

/*
 * Agency UID history.
 * This array saves the Smart Object's agency UID from which the ME is sent. This is used to prevent a ME to
 * go back to the originater (vicious circle).
 * The agency UIDs are in inverted chronological order: the oldest value is at index 0.
 */
static agencyUID_t agencyUID_history[2];

static agencyUID_t last_agencyUID;

/* Limited number of migrations */
static bool limited_migrations = false;
static uint32_t migration_count = 0;

/* SPID of the SOO.blind ME */
uint8_t SOO_blind_spid[SPID_SIZE] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x11, 0x8d };

/**
 * PRE-ACTIVATE
 */
int cb_pre_activate(soo_domcall_arg_t *args) {
	agency_ctl_args_t agency_ctl_args;

	DBG("Pre-activate %d\n", ME_domID());

	/* Retrieve the agency UID of the Smart Object on which the ME has migrated */
	agency_ctl_args.cmd = AG_AGENCY_UID;
	args->__agency_ctl(&agency_ctl_args);
	memcpy(&target_agencyUID, &agency_ctl_args.u.agencyUID_args.agencyUID, SOO_AGENCY_UID_SIZE);

	/* Detect if the ME has migrated, that is, it is now on another Smart Object */
	has_migrated = (agencyUID_is_valid(&last_agencyUID) && (memcmp(&target_agencyUID, &last_agencyUID, SOO_AGENCY_UID_SIZE) != 0));
	memcpy(&last_agencyUID, &target_agencyUID, SOO_AGENCY_UID_SIZE);

	outdoor_action_pre_activate();

	/* Retrieve the name of the Smart Object on which the ME has migrated */
	agency_ctl_args.cmd = AG_SOO_NAME;
	args->__agency_ctl(&agency_ctl_args);
	strcpy(target_soo_name, (const char *) agency_ctl_args.u.soo_name_args.soo_name);

	DBG("SOO." APP_NAME " ME now running on: ");
	DBG_BUFFER(&target_agencyUID, SOO_AGENCY_UID_SIZE);

	/* Check if the ME is not coming back to its source (vicious circle) */
	if (!memcmp(&target_agencyUID, &agencyUID_history[0], SOO_AGENCY_UID_SIZE)) {
		DBG("Back to the source\n");

		/* Kill the ME to avoid circularity */
		set_ME_state(ME_state_killed);

		return 0;
	}

	/* Ask if the Smart Object if a SOO.outdoor Smart Object */
	agency_ctl_args.u.devcaps_args.class = DEVCAPS_CLASS_DOMOTICS;
	agency_ctl_args.u.devcaps_args.devcaps = DEVCAP_WEATHER_DATA;
	agency_ctl_args.cmd = AG_CHECK_DEVCAPS;
	args->__agency_ctl(&agency_ctl_args);

	if (agency_ctl_args.u.devcaps_args.supported) {
		DBG("SOO." APP_NAME ": This is a SOO." APP_NAME " Smart Object\n");

		/* Tell that the expected devcaps have been found on this Smart Object */
		available_devcaps = true;
	} else {
		DBG("SOO." APP_NAME ": This is not a SOO." APP_NAME " Smart Object\n");

		/* Tell that the expected devcaps have not been found on this Smart Object */
		available_devcaps = false;

		/* If the devcaps are not available, set the ID to 0xff */
		my_id = 0xff;

		/* Ask if the Smart Object offers the dedicated remote application devcap */
		agency_ctl_args.u.devcaps_args.class = DEVCAPS_CLASS_APP;
		agency_ctl_args.u.devcaps_args.devcaps = DEVCAP_APP_OUTDOOR;
		agency_ctl_args.cmd = AG_CHECK_DEVCAPS;
		args->__agency_ctl(&agency_ctl_args);

		if (agency_ctl_args.u.devcaps_args.supported) {
			DBG("SOO." APP_NAME " remote application connected\n");

			/* Stay resident on the Smart Object */

			remote_app_connected = true;
		} else {
			DBG("No SOO." APP_NAME " remote application connected\n");

			/* Go to dormant state and migrate once */
			limited_migrations = true;
			migration_count = 0;

			set_ME_state(ME_state_dormant);
		}
	}

	return 0;
}

/**
 * PRE-PROPAGATE
 */
int cb_pre_propagate(soo_domcall_arg_t *args) {
	agency_ctl_args_t agency_ctl_args;
	pre_propagate_args_t *pre_propagate_args = (pre_propagate_args_t *) &args->u.pre_propagate_args;

	DBG("Pre-propagate %d\n", ME_domID());

	pre_propagate_args->propagate_status = 1;

	if (limited_migrations) {
		pre_propagate_args->propagate_status = 0;

		DBG("Limited number of migrations: %d/%d\n", migration_count, MAX_MIGRATION_COUNT);

		if ((get_ME_state() != ME_state_dormant) || (migration_count != MAX_MIGRATION_COUNT)) {
			pre_propagate_args->propagate_status = 1;
			migration_count++;
		} else
			set_ME_state(ME_state_killed);

		goto propagate;
	}

	/* SOO.outdoor: increment the age counter and reset the inertia counter */
	inc_age_reset_inertia(&outdoor_lock, outdoor_data->info.presence);

	/* SOO.outdoor: watch the inactive ages */
	watch_ages(&outdoor_lock,
			outdoor_data->info.presence, &outdoor_data->info,
			tmp_outdoor_info->presence, tmp_outdoor_info,
			sizeof(outdoor_info_t));

propagate:
	/* Save the previous source agency UID in the history */
	memcpy(&agencyUID_history[0], &agencyUID_history[1], SOO_AGENCY_UID_SIZE);

	/* Retrieve the agency UID of the Smart Object from which the ME will be sent */
	agency_ctl_args.cmd = AG_AGENCY_UID;
	args->__agency_ctl(&agency_ctl_args);
	memcpy(&agencyUID_history[1], &agency_ctl_args.u.agencyUID_args.agencyUID, SOO_AGENCY_UID_SIZE);

	DBG("SOO." APP_NAME " ME being sent by: ");
	DBG_BUFFER(&agencyUID_history[1], SOO_AGENCY_UID_SIZE);

	return 0;
}

/**
 * PRE-SUSPEND
 */
int cb_pre_suspend(soo_domcall_arg_t *args) {
	DBG("Pre-suspend %d\n", ME_domID());

	/* No propagation to the user space */
	return 0;
}

/**
 * COOPERATE
 */
int cb_cooperate(soo_domcall_arg_t *args) {
	cooperate_args_t *cooperate_args = (cooperate_args_t *) &args->u.cooperate_args;
	agency_ctl_args_t agency_ctl_args;
	bool SOO_outdoor_present = false;
	void *recv_data;
	size_t recv_data_size;
	unsigned int pfn;
	uint32_t i;

	DBG0("Cooperate\n");

	switch (cooperate_args->role) {
	case COOPERATE_INITIATOR:
		DBG("Cooperate: SOO." APP_NAME " Initiator %d\n", ME_domID());

		for (i = 0; i < MAX_ME_DOMAINS; i++) {
			/* Cooperation with a SOO.outdoor ME */
			if (!memcmp(cooperate_args->u.target_coop_slot[i].spid, SOO_outdoor_spid, SPID_SIZE)) {
				DBG("SOO." APP_NAME ": SOO.outdoor running on the Smart Object\n");
				SOO_outdoor_present = true;

				/* Only cooperate with MEs that accept to cooperate */
				if (!cooperate_args->u.target_coop_slot[i].spad.valid)
					continue;

				/*
				 * Prepare the arguments to transmit to the target ME:
				 * - Initiator's SPID
				 * - Initiator's SPAD capabilities
				 * - pfn of the localinfo
				 */
				agency_ctl_args.cmd = AG_COOPERATE;
				agency_ctl_args.slotID = cooperate_args->u.target_coop_slot[i].slotID;
				memcpy(agency_ctl_args.u.target_cooperate_args.spid, get_ME_desc()->spid, SPID_SIZE);
				memcpy(agency_ctl_args.u.target_cooperate_args.spad_caps, get_ME_desc()->spad.caps, SPAD_CAPS_SIZE);
				agency_ctl_args.u.target_cooperate_args.pfn.content = phys_to_pfn(virt_to_phys_pt((uint32_t) localinfo_data));
				args->__agency_ctl(&agency_ctl_args);
			}

			/* Cooperation with a SOO.blind ME */
			if (!memcmp(cooperate_args->u.target_coop_slot[i].spid, SOO_blind_spid, SPID_SIZE)) {
				DBG("SOO." APP_NAME ": SOO.blind running on the Smart Object\n");

				/* Only cooperate with MEs that accept to cooperate */
				if (!cooperate_args->u.target_coop_slot[i].spad.valid)
					continue;

				/*
				 * Prepare the arguments to transmit to the target ME:
				 * - Initiator's SPID
				 * - Initiator's SPAD capabilities
				 * - pfn of the localinfo
				 */
				agency_ctl_args.cmd = AG_COOPERATE;
				agency_ctl_args.slotID = cooperate_args->u.target_coop_slot[i].slotID;
				memcpy(agency_ctl_args.u.target_cooperate_args.spid, get_ME_desc()->spid, SPID_SIZE);
				memcpy(agency_ctl_args.u.target_cooperate_args.spad_caps, get_ME_desc()->spad.caps, SPAD_CAPS_SIZE);
				agency_ctl_args.u.target_cooperate_args.pfn.content = phys_to_pfn(virt_to_phys_pt((uint32_t) localinfo_data));
				args->__agency_ctl(&agency_ctl_args);
			}
		}

		/*
		 * There are two cases in which we have to kill the initiator SOO.outdoor ME:
		 * - There is already a SOO.outdoor ME running on the Smart Object, independently of its nature.
		 * - There is no SOO.outdoor ME running on a non-SOO.outdoor Smart Object (the expected devcaps are not present) and there
		 *   is no connected SOO.outdoor application.
		 */
		if (SOO_outdoor_present || (!SOO_outdoor_present && !available_devcaps && !remote_app_connected)) {
			agency_ctl_args.cmd = AG_KILL_ME;
			agency_ctl_args.slotID = ME_domID() + 1;
			DBG("Kill ME in slot ID: %d\n", ME_domID() + 1);
			args->__agency_ctl(&agency_ctl_args);
		}

		break;

	case COOPERATE_TARGET:
		DBG("Cooperate: SOO." APP_NAME " Target %d\n", ME_domID());

		DBG("SPID of the initiator: ");
		DBG_BUFFER(cooperate_args->u.initiator_coop.spid, SPID_SIZE);
		DBG("SPAD caps of the initiator: ");
		DBG_BUFFER(cooperate_args->u.initiator_coop.spad_caps, SPAD_CAPS_SIZE);

		/* Cooperation with a SOO.outdoor ME */
		if (!memcmp(cooperate_args->u.initiator_coop.spid, SOO_outdoor_spid, SPID_SIZE)) {
			DBG("Cooperation with SOO.outdoor\n");

			pfn = cooperate_args->u.initiator_coop.pfn.content;

			recv_data_size = DIV_ROUND_UP(sizeof(outdoor_info_t), PAGE_SIZE) * PAGE_SIZE;
			recv_data = (void *) io_map(pfn_to_phys(pfn), recv_data_size);

			merge_info(&outdoor_lock,
					outdoor_data->info.presence, &outdoor_data->info,
					((outdoor_data_t *) recv_data)->info.presence, recv_data,
					tmp_outdoor_info->presence, tmp_outdoor_info,
					sizeof(outdoor_info_t));

			io_unmap((uint32_t) recv_data);
		}

		break;

	default:
		lprintk("Cooperate: Bad role %d\n", cooperate_args->role);
		BUG();
	}

	return 0;
}

/**
 * PRE-RESUME
 */
int cb_pre_resume(soo_domcall_arg_t *args) {
	DBG("Pre-resume %d\n", ME_domID());

	return 0;
}

/**
 * POST-ACTIVATE
 */
int cb_post_activate(soo_domcall_arg_t *args) {
	DBG("Post-activate %d\n", ME_domID());

	/*
	 * At the first post-activate, initialize the outdoor descriptor of this Smart Object and start the
	 * threads.
	 */
	if (unlikely(!outdoor_initialized)) {
		/* Save the agency UID of the Smart Object on which the ME has been injected */
		memcpy(&origin_agencyUID, &target_agencyUID, SOO_AGENCY_UID_SIZE);

		/* Save the name of the Smart Object on which the ME has been injected */
		strcpy(origin_soo_name, target_soo_name);

		create_my_desc();
		get_my_id(outdoor_data->info.presence);

		outdoor_start_threads();

		outdoor_initialized = true;
	}

	outdoor_action_post_activate();

	return 0;
}

/**
 * LOCALINFO_UPDATE
 */
int cb_localinfo_update(void) {
	DBG("Localinfo update %d\n", ME_domID());

	return 0;
}

/**
 * FORCE_TERMINATE
 */
int cb_force_terminate(void) {
	DBG("ME %d force terminate, state: %d\n", ME_domID(), get_ME_state());

	/* We do nothing particular here for this ME, however we proceed with the normal termination of execution */
	set_ME_state(ME_state_terminated);

	return 0;
}

/**
 * KILL_ME
 */
int cb_kill_me(soo_domcall_arg_t *args) {
	DBG("Kill-ME %d\n", ME_domID());

	set_ME_state(ME_state_killed);

	return 0;
}

/**
 * Initializations for callback handling.
 */
void callbacks_init(void) {
	int nr_pages = DIV_ROUND_UP(sizeof(outdoor_data_t), PAGE_SIZE);

	DBG("Localinfo: size=%d, nr_pages=%d\n", sizeof(outdoor_data_t), nr_pages);
	/* Allocate localinfo */
	localinfo_data = (void *) get_contig_free_vpages(nr_pages);

	memcpy(&last_agencyUID, &null_agencyUID, SOO_AGENCY_UID_SIZE);
	memcpy(&agencyUID_history[0], &null_agencyUID, SOO_AGENCY_UID_SIZE);
	memcpy(&agencyUID_history[1], &null_agencyUID, SOO_AGENCY_UID_SIZE);

	/* Get ME SPID */
	memcpy(get_ME_desc()->spid, SOO_outdoor_spid, SPID_SIZE);
	lprintk("ME SPID: ");
	lprintk_buffer(SOO_outdoor_spid, SPID_SIZE);
}

