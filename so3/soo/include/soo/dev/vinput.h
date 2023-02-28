/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef VINPUT_H
#define VINPUT_H

#include <soo/ring.h>
#include <soo/grant_table.h>
#include <soo/vdevfront.h>

#define VINPUT_PACKET_SIZE	32

#define VINPUT_NAME		"vinput"
#define VINPUT_PREFIX		"[" VINPUT_NAME "-front] "

typedef struct {
	char buffer[VINPUT_PACKET_SIZE];
} vinput_request_t;

typedef struct  {
	unsigned int type;
	unsigned int code;
	int value;
} vinput_response_t;

/*
 * Generate ring structures and types.
 */
DEFINE_RING_TYPES(vinput, vinput_request_t, vinput_response_t);

/*
 * General structure for this virtual device (backend side)
 */
typedef struct {
	vdevfront_t vdevfront;

	vinput_front_ring_t ring;
	unsigned int irq;

	grant_ref_t ring_ref;
	grant_handle_t handle;
	uint32_t evtchn;

} vinput_t;

static inline vinput_t *to_vinput(struct vbus_device *vdev) {
	vdevfront_t *vdevback = dev_get_drvdata(vdev->dev);
	return container_of(vdevback, vinput_t, vdevfront);
}

#endif /* VINPUT_H */
