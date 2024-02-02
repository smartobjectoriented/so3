/*
 * Copyright (C) 2019 Baptiste Delporte <bonel@bonel.net>
 * Copyright (C) 2020 David Truan <david.truan@heig-vd.ch>
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

#include <soo/dev/vuihandler.h>

#include <me/outdoor/outdoor.h>

/* SPID of the SOO.outdoor ME */
uint8_t SOO_outdoor_spid[SPID_SIZE] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0xd0, 0x08 };

/* Null agency UID to check if an agency UID is valid */
agencyUID_t null_agencyUID = {
	.id = { 0 }
};

/* Localinfo buffer */
void *localinfo_data;

/*
 * - Global SOO.outdoor data
 * - Temporary SOO.outdoor info buffer for SOO presence operations
 * - Protection spinlock
 */
outdoor_data_t *outdoor_data;
outdoor_info_t *tmp_outdoor_info;
spinlock_t outdoor_lock;

/* ID of the Smart Object on which the ME is running. 0xff means that it has not been initialized yet. */
uint32_t my_id = 0xff;

/* Agency UID of the Smart Object on which the Entity has been injected */
agencyUID_t origin_agencyUID;

/* Name of the Smart Object on which the Entity has been injected */
char origin_soo_name[SOO_NAME_SIZE];

/* Agency UID of the Smart Object on which the Entity has migrated */
agencyUID_t target_agencyUID;

/* Name of the Smart Object on which the Entity has migrated */
char target_soo_name[SOO_NAME_SIZE];

/* Bool telling that the ME is in a Smart Object with the expected devcaps */
bool available_devcaps = false;

/* Bool telling that a remote application is connected */
bool remote_app_connected = false;

/* Detection of a ME that has migrated on another new Smart Object */
bool has_migrated = false;

/* vUIHandler packet */
static outdoor_vuihandler_pkt_t *outgoing_vuihandler_pkt;

/**
 * Interface with vWeather. This function is called when new weather data is available, in interrupt context.
 */
void weather_data_update_interrupt(void) {
	vweather_data_t *weather_data = vweather_get_data();

	/* Update local weather data */
	outdoor_data->info.outdoor[0].south_sun = weather_data->south_sun;
	outdoor_data->info.outdoor[0].west_sun = weather_data->west_sun;
	outdoor_data->info.outdoor[0].east_sun = weather_data->east_sun;
	outdoor_data->info.outdoor[0].light = weather_data->light;
	outdoor_data->info.outdoor[0].temperature = weather_data->temperature;
	outdoor_data->info.outdoor[0].wind = weather_data->wind;
	outdoor_data->info.outdoor[0].twilight = weather_data->twilight;
	outdoor_data->info.outdoor[0].rain = weather_data->rain;
	outdoor_data->info.outdoor[0].rain_intensity = weather_data->rain_intensity;

	outdoor_data->info.presence[0].change_count++;
}

/**
 * Reset a descriptor associated to a SOO.outdoor Smart Object.
 */
static void reset_outdoor_desc(uint8_t index) {
	outdoor_data->info.presence[index].active = false;
	memset(outdoor_data->info.presence[index].name, 0, MAX_NAME_SIZE);
	memset(outdoor_data->info.presence[index].agencyUID, 0, SOO_AGENCY_UID_SIZE);
	outdoor_data->info.presence[index].change_count = 0;
	outdoor_data->info.presence[index].age = 0;
	outdoor_data->info.presence[index].last_age = 0;
	outdoor_data->info.presence[index].inertia = 0;

	outdoor_data->info.outdoor[index].south_sun = 0;
	outdoor_data->info.outdoor[index].west_sun = 0;
	outdoor_data->info.outdoor[index].east_sun = 0;
	outdoor_data->info.outdoor[index].light = 0;
	outdoor_data->info.outdoor[index].temperature = 0;
	outdoor_data->info.outdoor[index].wind = 0;
	outdoor_data->info.outdoor[index].twilight = 0;
	outdoor_data->info.outdoor[index].rain = 0;
	outdoor_data->info.outdoor[index].rain_intensity = NO_RAIN;
}

/***** Interactions with the SOO Presence Pattern *****/

/**
 * Reset a descriptor.
 */
void reset_desc(void *local_info_ptr, uint32_t index) {
	reset_outdoor_desc(index);
}

/**
 * Copy a descriptor.
 */
void copy_desc(void *dest_info_ptr, uint32_t local_index, void *src_info_ptr, uint32_t remote_index) {
	outdoor_info_t *src_info = (outdoor_info_t *) src_info_ptr;

	memcpy(&outdoor_data->info.outdoor[local_index], &src_info->outdoor[remote_index], sizeof(outdoor_desc_t));
	memcpy(&outdoor_data->info.presence[local_index], &src_info->presence[remote_index], sizeof(soo_presence_data_t));
}

/**
 * Copy the information of an incoming SOO.* descriptor into the local global SOO.* info.
 * The whole descriptor is overwritten.
 */
void update_desc(uint8_t *agencyUID,
			void *local_info_ptr, uint32_t local_index,
			void *recv_data_ptr, uint32_t remote_index) {
	outdoor_info_t *incoming_info = (outdoor_info_t *) recv_data_ptr;

	memcpy(&outdoor_data->info.outdoor[local_index], &incoming_info->outdoor[remote_index], sizeof(outdoor_desc_t));
	memcpy(&outdoor_data->info.presence[local_index], &incoming_info->presence[remote_index], sizeof(soo_presence_data_t));
}

/**
 * If, after the merge, a SOO.outdoor update is necessary, bufferize the request so that the command will
 * be executed at the next post-activate.
 * This function is executed in domcall context. No call to completion() should be made here.
 */
void action_after_merge(void *local_info_ptr, uint32_t local_index, uint32_t remote_index) {
	/* Nothing to do */
}

/**
 * If, after the merge, a SOO.outdoor update is necessary, request it now.
 */
void outdoor_action_after_merge(uint32_t remote_index, uint32_t local_index) {
	/* Do nothing */
}

/**
 * Update the ID associated to this Smart Object in the SOO.outdoor info.
 */
void update_my_id(uint32_t new_id) {
	outdoor_data->info.my_id = my_id;
}

/***** Callbacks *****/

void outdoor_action_pre_activate(void) {
	/* Nothing to do */
}

/**
 * In post-activate, process the bufferized command if any.
 */
void outdoor_action_post_activate(void) {
	/* Nothing to do */
}

/***** Interactions with the vUIHandler interface *****/

void ui_interrupt(char *data, size_t size) {
	/* Do nothing as no command is expected to come from the UI interface */
}

/**
 * Function called when the connected application ME SPID changes.
 * - If the ME is running on a SOO.outdoor Smart Object: the ME stays resident.
 * - If the ME is on a non-SOO.outodor Smart Object: the ME must stay resident as long as the remote
 *   application is connected.
 */
void ui_update_app_spid(uint8_t *spid) {
	agency_ctl_args_t agency_ctl_args;

#ifdef DEBUG
	DBG("App ME SPID change: ");
	lprintk_buffer(spid, SPID_SIZE);
#endif /* DEBUG */

	if ((!available_devcaps) && (memcmp(spid, SOO_outdoor_spid, SPID_SIZE)) != 0) {
		DBG("Force Terminate ME in slot ID: %d\n", ME_domID() + 1);
		agency_ctl_args.cmd = AG_FORCE_TERMINATE;
		agency_ctl_args.slotID = ME_domID() + 1;
		agency_ctl(&agency_ctl_args);
	}
}


/**
 * Task that sends a vUIHandler packet with the SOO.outdoor info periodically.
 */
static int send_vuihandler_pkt_task_fn(void *arg) {
	unsigned long flags;

	while (1) {
		msleep(VUIHANDLER_PERIOD);

		spin_lock_irqsave(&outdoor_lock, flags);
		memcpy(outgoing_vuihandler_pkt->payload, &outdoor_data->info, sizeof(outdoor_info_t));
		spin_unlock_irqrestore(&outdoor_lock, flags);

		vuihandler_send(outgoing_vuihandler_pkt, OUTDOOR_VUIHANDLER_HEADER_SIZE + OUTDOOR_VUIHANDLER_DATA_SIZE);
	}

	return 0;
}

/**
 * Start the threads.
 */
void outdoor_start_threads(void) {
	kernel_thread(send_vuihandler_pkt_task_fn, "vUIHandler", NULL, 0);
}

/**
 * Create the SOO.outdoor descriptor of this Smart Object.
 */
void create_my_desc(void) {
	reset_outdoor_desc(0);

	memcpy(outdoor_data->info.presence[0].agencyUID, &origin_agencyUID, SOO_AGENCY_UID_SIZE);
	strcpy((char *) outdoor_data->info.presence[0].name, origin_soo_name);
	outdoor_data->info.presence[0].active = true;

	memcpy(outdoor_data->origin_agencyUID, &origin_agencyUID, SOO_AGENCY_UID_SIZE);
}

/**
 * Dump the contents of the SOO.outdoor info.
 * For debugging purposes.
 */
void outdoor_dump(void) {
	uint32_t i;

	spin_lock(&outdoor_lock);
	memcpy(tmp_outdoor_info, &outdoor_data->info, sizeof(outdoor_info_t));
	spin_unlock(&outdoor_lock);

	lprintk("My ID: %d\n", my_id);

	for (i = 0; i < MAX_DESC; i++) {
		if (!tmp_outdoor_info->presence[i].active)
			continue;

		lprintk("%d:\n", i);
		lprintk("    Name: %s\n", tmp_outdoor_info->presence[i].name);
		lprintk("    Agency UID: "); lprintk_buffer(tmp_outdoor_info->presence[i].agencyUID, SOO_AGENCY_UID_SIZE);
		lprintk("    Age: %d\n", tmp_outdoor_info->presence[i].age);
		lprintk("    Inertia: %d\n", tmp_outdoor_info->presence[i].inertia);
		lprintk("    South Sun: %dklx\n", tmp_outdoor_info->outdoor[i].south_sun);
		lprintk("    West Sun: %dklx\n", tmp_outdoor_info->outdoor[i].west_sun);
		lprintk("    East Sun: %dklx\n", tmp_outdoor_info->outdoor[i].east_sun);
		lprintk("    Light: %dlx\n", tmp_outdoor_info->outdoor[i].light);
		lprintk("    Temperature: %d x 1E-1°C\n", tmp_outdoor_info->outdoor[i].temperature);
		lprintk("    Wind: %d x 1E-1m/s\n", tmp_outdoor_info->outdoor[i].wind);
		lprintk("    Twilight: %c\n", (tmp_outdoor_info->outdoor[i].twilight) ? 'y' : 'n');
		lprintk("    Rain: %c\n", (tmp_outdoor_info->outdoor[i].rain) ? 'y' : 'n');
		lprintk("    Rain intensity: ");
		switch (tmp_outdoor_info->outdoor[i].rain_intensity) {
		case NO_RAIN:
			lprintk("no rain\n");
			break;
		case LIGHT_RAIN:
			lprintk("light rain\n");
			break;
		case MODERATE_RAIN:
			lprintk("moderate rain\n");
			break;
		case HEAVY_RAIN:
			lprintk("heavy rain\n");
			break;
		default:
			lprintk("?\n");
			break;
		}
		lprintk("\n");
	}
}

/**
 * Application initialization.
 */
void outdoor_init(void) {
	uint32_t i;

	/* Data buffer allocation */
	outdoor_data = (outdoor_data_t *) localinfo_data;
	memset(outdoor_data, 0, sizeof(outdoor_data_t));
	tmp_outdoor_info = (outdoor_info_t *) malloc(sizeof(outdoor_info_t));
	memset(tmp_outdoor_info, 0, sizeof(outdoor_info_t));
	spin_lock_init(&outdoor_lock);

	/* Reset all SOO.outdoor descriptors */
	for (i = 0; i < MAX_DESC; i++)
		reset_outdoor_desc(i);

	vuihandler_register_callback(ui_update_app_spid, ui_interrupt);
	vweather_register_interrupt(weather_data_update_interrupt);

	/* Allocate the outgoing vUIHandler packet */
	outgoing_vuihandler_pkt = (outdoor_vuihandler_pkt_t *) malloc(OUTDOOR_VUIHANDLER_HEADER_SIZE + OUTDOOR_VUIHANDLER_DATA_SIZE);

	DBG("SOO." APP_NAME " ME ready\n");
}

/*
 * The main application of the ME is executed right after the bootstrap. It may be empty since activities can be triggered
 * by external events based on frontend activities.
 */
int app_thread_main(void *args) {

	lprintk("SOO." APP_NAME " Mobile Entity booting ...\n");

	soo_guest_activity_init();

	callbacks_init();

	/* Initialize the application */
	outdoor_init();

	lprintk("SOO." APP_NAME " Mobile Entity -- Copyright (c) 2016-2020 REDS Institute (HEIG-VD)\n\n");

	DBG("ME running as domain %d\n", ME_domID());

	return 0;
}
