/*
 * Copyright (C) 2018-2019 Baptiste Delporte <bonel@bonel.net>
 * Copyright (C) 2018-2019 David Truan <david.truan@heig-vd.ch>
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

#ifndef OUTDOOR_H
#define OUTDOOR_H

#include <types.h>
#include <ioctl.h>

#include <me/eco_stability.h>

#include <soo/soo.h>

#include <soo/dev/vweather.h>

#define APP_NAME	"outdoor"

typedef struct {

	/* Weather data */
	uint32_t		south_sun;	/* South Sun in klx */
	uint32_t		west_sun;	/* West Sun in klx */
	uint32_t		east_sun;	/* East Sun in klx */
	uint32_t		light;		/* Light in lx */
	int32_t			temperature;	/* Temperature in 1E-1 Celsius degrees */
	uint32_t		wind;		/* Wind speed in 1E-1 m/s */
	uint8_t			twilight;	/* Twilight: 1 or 0 */
	uint8_t			rain;		/* Rain: 1 or 0 */
	rain_intensity_t	rain_intensity;	/* Rain intensity */

} outdoor_desc_t;

/* The global outdoor info contains blind descriptors and the ID of this Smart Object */
typedef struct {
	outdoor_desc_t	outdoor[MAX_DESC];
	uint8_t		my_id;

	/* SOO Presence Behavioural Pattern data */
	soo_presence_data_t	presence[MAX_DESC];
} outdoor_info_t;

/* The global blind data contains the global blind info */
typedef struct {
	outdoor_info_t	info;
	uint8_t		origin_agencyUID[SOO_AGENCY_UID_SIZE];
} outdoor_data_t;

typedef struct {
    uint8_t		type;
    uint8_t		payload[0];
} outdoor_vuihandler_pkt_t;

#define OUTDOOR_VUIHANDLER_HEADER_SIZE	sizeof(outdoor_vuihandler_pkt_t)
#define OUTDOOR_VUIHANDLER_DATA_SIZE	sizeof(outdoor_info_t)

extern uint8_t SOO_outdoor_spid[SPID_SIZE];

extern void *localinfo_data;
extern outdoor_data_t *outdoor_data;
extern outdoor_info_t *tmp_outdoor_info;
extern outdoor_desc_t *tmp_outdoor_desc;

extern uint32_t my_id;

extern agencyUID_t origin_agencyUID;
extern char origin_soo_name[SOO_NAME_SIZE];
extern agencyUID_t target_agencyUID;
extern char target_soo_name[SOO_NAME_SIZE];

extern bool available_devcaps;
extern bool remote_app_connected;
extern bool can_migrate;
extern bool has_migrated;

extern spinlock_t outdoor_lock;

void outdoor_start_threads(void);
void create_my_desc(void);

void outdoor_enable_vuihandler(void);
void outdoor_disable_vuihandler(void);

void outdoor_init(void);

void outdoor_dump(void);

void outdoor_action_pre_activate(void);
void outdoor_action_post_activate(void);

#endif /* OUTDOOR_H */
