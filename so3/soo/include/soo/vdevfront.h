/*
 * Copyright (C) 2020 Daniel Rossier <daniel.rossier@soo.tech>
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

#ifndef VDEVFRONT_H
#define VDEVFRONT_H

#include <completion.h>
#include <mutex.h>

#include <soo/vbus.h>

#include <device/device.h>

#include <asm/atomic.h>

struct vdrvfront;

struct vdevfront {

	/* Frontend activity related declarations - managed by vdevfront generic code */

	atomic_t processing_count;

	/* Protection against shutdown (or other) */
	struct mutex processing_lock;

	/* Synchronization between ongoing processing and suspend/closing */
	struct completion sync;
};
typedef struct vdevfront vdevfront_t;

struct vdrvfront {

	struct vbus_driver vdrv;

	void (*probe)(struct vbus_device *vdev);

	void (*reconfiguring)(struct vbus_device *vdev);
	void (*shutdown)(struct vbus_device *vdev);
	void (*closed)(struct vbus_device *vdev);
	void (*suspend)(struct vbus_device *vdev);
	void (*resume)(struct vbus_device *vdev);
	void (*connected)(struct vbus_device *vdev);

};
typedef struct vdrvfront vdrvfront_t;

static inline vdrvfront_t *to_vdrvfront(struct vbus_device *vdev) {
	struct vbus_driver *vdrv = vdev->vdrv;
	return container_of(vdrv, vdrvfront_t, vdrv);
}

void vdevfront_init(char *name, vdrvfront_t *vdrvfront);
bool vdevfront_processing_begin(struct vbus_device *vdev);
void vdevfront_processing_end(struct vbus_device *vdev);

#endif /* VDEVFRONT_H */




