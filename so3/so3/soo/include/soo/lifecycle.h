/*
 * Copyright (C) 2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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

#ifndef SOO_LIFECYCLE_H
#define SOO_LIFECYCLE_H

#include <soo/uapi/soo.h>

/*
 * /dev/soo: the capsule's lifecycle as seen from its user space.
 *
 * SOO_IOC_WAIT blocks until the next callback; SOO_IOC_ANSWER answers it.
 * read() gives the data of the capsule cooperating with us (cooperate_cb),
 * write() sets ours for the capsules we cooperate with (cooperate).
 */

#define SOO_IOC_WAIT 1
#define SOO_IOC_ANSWER 2
#define SOO_IOC_SET_SUMMARY 3
#define SOO_IOC_STOP_PROPAGATE 4
#define SOO_IOC_GET_IDS 5

struct soo_callback {
	dc_event_t event;
	s3c_lc_in_t in;
};

struct soo_answer {
	uint32_t flags;
	uint32_t kill_mask;
	uint32_t coop_mask;
};

struct soo_ids {
	uint64_t capsule_uid;
	uint64_t spid;
	uint32_t slotID;
	uint64_t device_uid;
	char devices[S3C_LC_DEVICES_LEN];
};

void lifecycle_init(void);
void lifecycle_callback(dc_event_t dc_event);
void lifecycle_user(dc_event_t dc_event);

void cb_pre_activate(s3c_lifecycle_t *lifecycle);
void cb_cooperate(s3c_lifecycle_t *lifecycle);
void cb_cooperate_cb(s3c_lifecycle_t *lifecycle);

#endif /* SOO_LIFECYCLE_H */
