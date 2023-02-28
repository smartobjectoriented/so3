/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2016-2018 Baptiste Delporte <bonel@bonel.net>
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

#include <types.h>
#include <string.h>

#include <soo/gnttab.h>
#include <soo/grant_table.h>
#include <soo/hypervisor.h>
#include <soo/avz.h>
#include <soo/console.h>
#include <soo/vbus.h>

#include <avz/uapi/event_channel.h>

const char *vbus_strstate(enum vbus_state state)
{
	static const char *const name[] = {
		[ VbusStateUnknown      ] = "Unknown",
		[ VbusStateInitialising ] = "Initialising",
		[ VbusStateInitWait     ] = "InitWait",
		[ VbusStateInitialised  ] = "Initialised",
		[ VbusStateConnected    ] = "Connected",
		[ VbusStateClosing      ] = "Closing",
		[ VbusStateClosed	      ] = "Closed",
		[ VbusStateReconfiguring] = "Reconfiguring",
		[ VbusStateReconfigured ] = "Reconfigured",
		[ VbusStateSuspending   ] = "Suspending",
		[ VbusStateSuspended    ] = "Suspended",
		[ VbusStateResuming     ] = "Resuming",
	};
	return (state < ARRAY_SIZE(name)) ? name[state] : "INVALID";
}

/**
 * vbus_watch_path - register a watch
 * @dev: vbus device
 * @path: path to watch
 * @watch: watch to register
 * @callback: callback to register
 *
 * Register a @watch on the given path, using the given vbus_watch structure
 * for storage, and the given @callback function as the callback.  Return 0 on
 * success, or -errno on error.  On success, the given @path will be saved as
 * @watch->node, and remains the caller's to free.  On error, @watch->node will
 * be NULL, the device will switch to %VbusStateClosing, and the error will
 * be saved in the store.
 */
void vbus_watch_path(struct vbus_device *dev, char *path, struct vbus_watch *watch, void (*callback)(struct vbus_watch *))
{
	watch->node = path;
	watch->callback = callback;

	register_vbus_watch(watch);
}


/**
 * vbus_watch_pathfmt - register a watch on a sprintf-formatted path
 * @dev: vbus device
 * @watch: watch to register
 * @callback: callback to register
 * @pathfmt: format of path to watch
 *
 * Register a watch on the given @path, using the given vbus_watch
 * structure for storage, and the given @callback function as the callback.
 * Return 0 on success, or -errno on error.  On success, the watched path
 * (@path/@path2) will be saved as @watch->node, and becomes the caller's to
 * kfree().  On error, watch->node will be NULL, so the caller has nothing to
 * free, the device will switch to %VbusStateClosing, and the error will be
 * saved in the store.
 */
void vbus_watch_pathfmt(struct vbus_device *dev, struct vbus_watch *watch, void (*callback)(struct vbus_watch *), const char *pathfmt, ...)
{
	va_list ap;
	char *path;

	va_start(ap, pathfmt);
	path = kvasprintf(pathfmt, ap);
	va_end(ap);

	if (!path) {
		lprintk("%s - line %d: Allocating path for watch failed for device %s\n", __func__, __LINE__, dev->nodename);
		BUG();
	}
	vbus_watch_path(dev, path, watch, callback);

}

/**
 * vbus_grant_ring
 * @dev: vbus device
 * @ring_mfn: mfn of ring to grant

 * Grant access to the given @ring_mfn to the peer of the given device.  Return
 * 0 on success, or -errno on error.  On error, the device will switch to
 * VbusStateClosing, and the error will be saved in the store.
 */
int vbus_grant_ring(struct vbus_device *dev, unsigned long ring_pfn)
{
	return gnttab_grant_foreign_access(dev->otherend_id, ring_pfn, 0);
}

/**
 * Allocate an event channel for the given vbus_device, assigning the newly
 * created local evtchn to *evtchn.  Return 0 on success, or -errno on error.  On
 * error, the device will switch to VbusStateClosing, and the error will be
 * saved in the store.
 */
void vbus_alloc_evtchn(struct vbus_device *dev, uint32_t *evtchn)
{
	struct evtchn_alloc_unbound alloc_unbound;

	alloc_unbound.dom = DOMID_SELF;

	if (dev->realtime)
		alloc_unbound.remote_dom = DOMID_AGENCY_RT;
	else
		alloc_unbound.remote_dom = dev->otherend_id;

	hypercall_trampoline(__HYPERVISOR_event_channel_op, EVTCHNOP_alloc_unbound, (long) &alloc_unbound, 0, 0);

	*evtchn = alloc_unbound.evtchn;
}


/**
 * Bind to an existing interdomain event channel in another domain. Returns 0
 * on success and stores the local evtchn in *evtchn. On error, returns -errno,
 * switches the device to VbusStateClosing, and saves the error in VBstore.
 */
void vbus_bind_evtchn(struct vbus_device *dev, uint32_t remote_evtchn, uint32_t *evtchn)
{
	struct evtchn_bind_interdomain bind_interdomain;

	bind_interdomain.remote_dom = dev->otherend_id;
	bind_interdomain.remote_evtchn = remote_evtchn;

	hypercall_trampoline(__HYPERVISOR_event_channel_op, EVTCHNOP_bind_interdomain, (long) &bind_interdomain, 0, 0);

	*evtchn = bind_interdomain.local_evtchn;
	DBG("%s: got local evtchn: %d for remote evtchn: %d\n", __func__, *evtchn, remote_evtchn);
}

/**
 * Free an existing event channel. Returns 0 on success or -errno on error.
 */
void vbus_free_evtchn(struct vbus_device *dev, uint32_t evtchn)
{
	struct evtchn_close close;

	close.evtchn = evtchn;

	hypercall_trampoline(__HYPERVISOR_event_channel_op, EVTCHNOP_close, (long) &close, 0, 0);
}

/**
 * vbus_read_driver_state
 * @path: path for driver
 *
 * Return the state of the driver rooted at the given store path, or
 * VbusStateUnknown if no state can be read.
 */
enum vbus_state vbus_read_driver_state(const char *path)
{
	enum vbus_state result;
	bool found;

	found = vbus_gather(VBT_NIL, path, "state", "%d", &result, NULL);

	if (!found)
		result = VbusStateUnknown;

	return result;
}

/**
 * vbus_read_driver_realtime
 * @path: path for driver
 *
 * Return the value to indicate if the driver is realtime or not.
 */
bool vbus_read_driver_realtime(const char *path)
{
	int val;

	vbus_gather(VBT_NIL, path, "realtime", "%d", &val, NULL);

	return val;
}

