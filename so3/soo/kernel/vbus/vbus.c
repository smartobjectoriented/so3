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
#include <heap.h>
#include <list.h>

#include <device/device.h>

#include <soo/hypervisor.h>
#include <soo/avz.h>
#include <soo/vbus.h>
#include <soo/evtchn.h>
#include <soo/vbstore.h>

#include <soo/soo.h>
#include <soo/console.h>
#include <soo/debug.h>
#include <soo/debug/dbgvar.h>
#include <soo/debug/logbool.h>

#define SYNC_BACKFRONT_COMPLETE		0
#define SYNC_BACKFRONT_SUSPEND		1
#define SYNC_BACKFRONT_RESUME		2
#define SYNC_BACKFRONT_SUSPENDED	3

#define VBUS_TIMEOUT	120

/* Event channels used for directcomm channel between agency and ME */
unsigned int dc_evtchn;
spinlock_t dc_lock;

/* List of device drivers */
LIST_HEAD(vbus_drivers);

/*
 * Walk through the list of vbus device drivers and perform an action.
 * When the action returns 1, we stop the walking.
 */
void vbus_drivers_for_each(void *data, int (*fn)(struct vbus_driver *, void *)) {
	struct list_head *pos;
	struct vbus_driver *vdrv;

	list_for_each(pos, &vbus_drivers)
	{
		vdrv = list_entry(pos, struct vbus_driver, list);
		if (fn(vdrv, data) == 1)
			return ;
	}
}

static int __vbus_switch_state(struct vbus_device *vdev, enum vbus_state state, bool force)
{
	/*
	 * We check whether the state is currently set to the given value, and if not, then the state is set.  We don't want to unconditionally
	 * write the given state, because we don't want to fire watches unnecessarily.  Furthermore, if the node has gone, we don't write
	 * to it, as the device will be tearing down, and we don't want to resurrect that directory.
	 *
	 */

	struct vbus_transaction vbt;

	if (!force && (state == vdev->state))
		return 0;

	/* Make visible the new state to the rest of world NOW...
	 * The remaining code is highly asynchronous...
	 */

	/* We make the strong assumption that the state can NOT be changed in parallel. The state machine is well-defined
	 * and simultaneous changes should simply NEVER happen.
	 */

	vdev->state = state;

	smp_mb();

	vbus_transaction_start(&vbt);
	vbus_printf(vbt, vdev->nodename, "state", "%d", state);
	vbus_transaction_end(vbt);

	return 0;
}

/**
 * vbus_switch_state
 * @vdev: vbus device
 * @state: new state
 *
 * Advertise in the store a change of the given driver to the given new_state.
 * Return 0 on success, or -errno on error.
 */
static int vbus_switch_state(struct vbus_device *vdev, enum vbus_state state)
{
	DBG("--> changing state of %s from %d to %d\n", vdev->nodename, vdev->state, state);

	return __vbus_switch_state(vdev, state, false);
}

/*
 * Remove the watch associated to remove device (especially useful for monitoring the state).
 */
void free_otherend_watch(struct vbus_device *vdev, bool with_vbus) {

	if (vdev->otherend_watch.node) {

		if (with_vbus)
			unregister_vbus_watch(&vdev->otherend_watch);
		else
			unregister_vbus_watch_without_vbus(&vdev->otherend_watch);

		free(vdev->otherend_watch.node);
		vdev->otherend_watch.node = NULL;

		/* No watch on otherend, and no interactions anymoire. */
		vdev->otherend[0] = 0;
	}
}

/*
 * Specific watch register function to focus on the state of a device on the other side.
 */
void watch_otherend(struct vbus_device *vdev) {
	vbus_watch_pathfmt(vdev, &vdev->otherend_watch, vdev->vbus->otherend_changed, "%s/%s", vdev->otherend, "state");
}

/*
 * Announce ourself to the otherend managed device. We mainly prepare to set up a watch on the device state.
 */
static void talk_to_otherend(struct vbus_device *vdev) {
	struct vbus_driver *vdrv = vdev->vdrv;

	BUG_ON(vdev->otherend[0] != 0);
	BUG_ON(vdev->otherend_watch.node != NULL);
	
	vdrv->read_otherend_details(vdev);

	/* Set up watch on state of otherend */
	watch_otherend(vdev);
}

void vbus_read_otherend_details(struct vbus_device *vdev, char *id_node, char *path_node) {
	vbus_gather(VBT_NIL, vdev->nodename, id_node, "%i", &vdev->otherend_id, path_node, "%s", vdev->otherend, NULL);
}
/*
 * The following function is called either in the backend OR the frontend.
 * On the backend side, it may run on CPU #0 (non-RT) or CPU #1 if the backend is configured as realtime.
 */
void vbus_otherend_changed(struct vbus_watch *watch) {
	struct vbus_device *vdev = container_of(watch, struct vbus_device, otherend_watch);
	struct vbus_driver *vdrv = vdev->vdrv;

	enum vbus_state state;

	state = vbus_read_driver_state(vdev->otherend);

        DBG("On domID: %d, otherend changed / device: %s  state: %d, CPU %d\n", ME_domID(), vdev->nodename, state, smp_processor_id());
	
        /*
         * Set immediately the frontend as connected if the backend announces to be connected.
         * It will help in the processing of connected function callback in the upper layers.
         */

	if (state == VbusStateConnected)
		vbus_switch_state(vdev, VbusStateConnected);

	/* We do not want to call a callback in a frontend on InitWait. This is
	 * a state issued from the backend to tell the frontend it can be probed.
	 */
	if ((vdrv->otherend_changed) && (state != VbusStateInitWait))
		vdrv->otherend_changed(vdev, state);

	BUG_ON(local_irq_is_disabled());

	switch (state) {

	case VbusStateInitWait:

		/* Check if we are suspended (before migration). In this case, we do nothing since the backend will
		 * set its state in resuming later on.
		 */
		if (vdev->state != VbusStateSuspended) {
			/*
			 * We set up the watch on the state at this time since the frontend probe will lead to
			 * state Initialised, which will trigger rather quickly a Connected state event from the backend.
			 * We have to be ready to process it.
			 */
			DBG("%s: Backend probed device: %s, now the frontend will be probing on its side.\n", __func__, vdev->nodename);

			vdrv->probe(vdev);

			vbus_switch_state(vdev, VbusStateInitialised);
		}
		break;


	case VbusStateSuspending:
		vbus_switch_state(vdev, VbusStateSuspended);
		break;

		/*
		 * Check for a final action.
		 *
		 * The backend has been shut down. Once the frontend has finished its work,
		 * we need to release the pending completion lock.
		 */
	case VbusStateClosed:
		/* In the frontend, we are completing the closing. */
		complete(&vdev->down);
		break;

	case VbusStateReconfiguring:
		vbus_switch_state(vdev, VbusStateReconfigured);
		break;

	case VbusStateResuming:
		vbus_switch_state(vdev, VbusStateConnected);
		break;

	default:
		break;

	}
}

/*
 * vbus_dev_probe() is called by the Linux device subsystem when probing a device
 */
int vbus_dev_probe(struct vbus_device *vdev)
{
	struct vbus_driver *vdrv = vdev->vdrv;

	DBG("%s\n", vdev->nodename);

	if (!vdrv->probe)
		BUG();

	init_completion(&vdev->down);
	init_completion(&vdev->sync_backfront);

	DBG("ME #%d  talk_to_otherend: %s\n", ME_domID(), vdev->nodename);

	talk_to_otherend(vdev);

	/* On frontend side, the probe will be executed as soon as the backend reaches the state InitWait */

	return 0;
}

int vbus_dev_remove(struct vbus_device *vdev)
{
	unsigned int dir_exists;

	/*
	 * If the ME is running on a Smart Object which does not offer all the backends matching the ME's frontends,
	 * some frontend related entries may not have been created. We must check here if the entry matching the dev
	 * to remove exists.
	 */
	dir_exists = vbus_directory_exists(VBT_NIL, vdev->otherend_watch.node, "");
	if (dir_exists) {
		DBG("%s", vdev->nodename);

		/* Remove the watch on the remote device. */
		free_otherend_watch(vdev, true);

		/* Definitively remove everything about this device */
		free(vdev);
	}

	return 0;
}

/*
 * Shutdown a device.
 */
void vbus_dev_shutdown(struct vbus_device *vdev)
{
	struct vbus_driver *vdrv = vdev->vdrv;
	unsigned int dir_exists;

	DBG("%s", vdev->nodename);

	dir_exists = vbus_directory_exists(VBT_NIL, vdev->otherend_watch.node, "");
	if (dir_exists) {
		if (vdev->state != VbusStateConnected) {
			printk("%s: %s: %s != Connected, skipping\n", __func__, vdev->nodename, vbus_strstate(vdev->state));
			BUG();
		}

		if (vdrv->shutdown != NULL)
			vdrv->shutdown(vdev);

		vbus_switch_state(vdev, VbusStateClosing);

		wait_for_completion(&vdev->down);
	}
}

struct vb_find_info {
	struct vbus_device *vdev;
	const char *nodename;
};

static int cmp_dev(struct vbus_device *vdev, void *data)
{
	struct vb_find_info *info = data;

	if (!strcmp(vdev->nodename, info->nodename)) {
		info->vdev = vdev;
		return 1;
	}
	return 0;
}

struct vbus_device *vbus_device_find(const char *nodename)
{
	struct vb_find_info info = { .vdev = NULL, .nodename = nodename };

	frontend_for_each(&info, cmp_dev);

	return info.vdev;
}

/* Driver management */

int vbus_match(struct vbus_driver *vdrv, void *data) {
	struct vbus_device *vdev = (struct vbus_device *) data;

	if (!strcmp(vdrv->devicetype, vdev->devicetype)) {
		vdev->vdrv = vdrv;
		vbus_dev_probe(vdev);
		return 1;
	}

	return 0;
}

/*
 * Create a new node and initialize basic structure (vdev)
 */
static struct vbus_device *vbus_probe_node(struct vbus_type *bus, const char *type, const char *nodename, char const *compat)
{
	char devname[VBUS_ID_SIZE];
	int err;
	struct vbus_device *vdev;
	enum vbus_state state;
	bool realtime;

	state = vbus_read_driver_state(nodename);
	realtime = vbus_read_driver_realtime(nodename);

	/* If the backend driver entry exists, but no frontend is using it, there is no
	 * vbstore entry related to the state and we simply skip it.
	 */
	if (state == VbusStateUnknown)
		return 0;

	BUG_ON(state != VbusStateInitialising);

	vdev = malloc(sizeof(struct vbus_device));
	BUG_ON(!vdev);

	memset(vdev, 0, sizeof(struct vbus_device));

	vdev->state = VbusStateInitialising;

	vdev->resuming = 0;
	vdev->realtime = realtime;

	vdev->vdrv = NULL;
	vdev->vbus = bus;

	vdev->dev = find_device(compat);
	if (!vdev->dev) {
		printk("## Failed at finding device %s\n", compat);
		BUG();
	}

	strcpy(vdev->nodename, nodename);
	strcpy(vdev->devicetype, type);

	err = bus->get_bus_id(devname, vdev->nodename);
	if (err)
		BUG();
	/*
	 * Register with generic device framework.
	 * The link with the driver is also done at this moment.
	 */

	/* Check for a driver and device matching */
	vbus_drivers_for_each(vdev, vbus_match);

	return vdev;
}


int vbus_register_driver_common(struct vbus_driver *vdrv)
{
	DBG("Registering driver name: %s\n", vdrv->name);

	/* Add the new driver to the main list */
	list_add_tail(&vdrv->list, &vbus_drivers);

	return 0;
}

/******************/

void vbus_dev_changed(const char *node, char *type, struct vbus_type *bus, const char *compat) {
	struct vbus_device *vdev;

	/*
	 * Either the device does not exist (backend or frontend) and the dev must be allocated, initialized
	 * and probed via the dev subsystem of Linux, OR the device exists (after migration)
	 * and in this case, the device exists on the frontend side only, and we only have to "talk_to_otherend" to
	 * set up the watch on its state (and retrieve the otherend id and name).
	 */

	vdev = vbus_device_find(node);
	if (!vdev) {
		vdev = vbus_probe_node(bus, type, node, compat);

		/* Add the new device to the main list */
		add_new_dev(vdev);

	} else {

		BUG_ON(ME_domID() == DOMID_AGENCY);

		/* Update the our state in vbstore. */
		/* We force the update, this will not trigger a watch since the watch is set right afterwards */
		 __vbus_switch_state(vdev, vdev->state, true);

		/* Setting the watch on the state */
		talk_to_otherend(vdev);
	}
}

/*
 * Perform a bottom half (deferred) processing on the receival of dc_event.
 * Here, we avoid to use a worqueue. Prefer thread instead, it will be also easier to manage with SO3.
 */
static irq_return_t directcomm_isr_thread(int irq, void *data) {
	dc_event_t dc_event;

	dc_event = atomic_read(&avz_shared->dc_event);

	/* Reset the dc_event now so that the domain can send another dc_event */
	atomic_set((atomic_t *) &avz_shared->dc_event, DC_NO_EVENT);

	perform_task(dc_event);

	return IRQ_COMPLETED;
}

/*
 * Interrupt routine for direct communication event channel
 * IRQs are off
 */
static irq_return_t directcomm_isr(int irq, void *data) {
	dc_event_t dc_event;

	dc_event = atomic_read(&avz_shared->dc_event);

	DBG("(ME domid %d): Received directcomm interrupt for event: %d\n", ME_domID(), avz_shared->dc_event);

	/* We should not receive twice a same dc_event, before it has been fully processed. */
	BUG_ON(atomic_read(&dc_incoming_domID[dc_event]) != -1);

	atomic_set(&dc_incoming_domID[dc_event], DOMID_AGENCY); /* At the moment, only from the agency */

	/* Work to be done in ME */

	switch (dc_event) {

	case DC_RESUME:
	case DC_SUSPEND:
	case DC_PRE_SUSPEND:
	case DC_FORCE_TERMINATE:
	case DC_POST_ACTIVATE:
	case DC_TRIGGER_DEV_PROBE:
	case DC_TRIGGER_LOCAL_COOPERATION:
	
		/* Check if it is the response to a dc_event. */
		if (atomic_read(&dc_outgoing_domID[dc_event]) != -1) {
			dc_stable(dc_event);
			break; /* Out of the switch */
		}

		/* Start the deferred thread */
		return IRQ_BOTTOM;

	default:
		printk("(ME) %s: something weird happened, directcomm interrupt was triggered, but no DC event was configured !\n", __func__);
		break;

	}

	/* Reset the dc_event now so that the domain can send another dc_event */
	atomic_set((atomic_t *) &avz_shared->dc_event, DC_NO_EVENT);

	return IRQ_COMPLETED;
}

/*
 * Vbus initialization function.
 */
void vbus_init(void)
{
	int res;
	char buf[20];
	struct vbus_transaction vbt;

	spin_lock_init(&dc_lock);

	vbstore_me_init();

	/* Set up the direct communication channel for post-migration activities
	 * previously established by dom0.
	 */

	vbus_transaction_start(&vbt);

	sprintf(buf, "soo/directcomm/%d", ME_domID());

	res = vbus_scanf(vbt, buf, "event-channel", "%d", &dc_evtchn);

	if (res != 1) {
		printk("%s: reading soo/directcomm failed. Error code: %d\n", __func__, res);
		BUG();
	}

	vbus_transaction_end(vbt);

	/* Binding the irqhandler to the eventchannel */
	DBG("%s: setting up the direct comm event channel (%d) ...\n", __func__, dc_evtchn);
	res = bind_interdomain_evtchn_to_irqhandler(DOMID_AGENCY, dc_evtchn, directcomm_isr, directcomm_isr_thread, NULL);

	if (res <= 0) {
		printk("Error: bind_evtchn_to_irqhandler failed");
		BUG();
	}

	dc_evtchn = evtchn_from_irq(res);
	DBG("%s: local event channel bound to directcomm towards non-RT Agency : %d\n", __func__, dc_evtchn);

	DBG("vbus_init OK!\n");
}

/*
 * DOMCALL_sync_directcomm
 */
int do_sync_directcomm(void *arg)
{
	struct DOMCALL_directcomm_args *args = arg;

	args->directcomm_evtchn = dc_evtchn;

	return 0;
}
