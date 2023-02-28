/*
 * Copyright (C) 2018-2019 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2018-2019 Baptiste Delporte <bonel@bonel.net>
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

#include <soo/gnttab.h>
#include <soo/evtchn.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/console.h>
#include <soo/debug.h>

#include <soo/dev/vdoga12v6nm.h>

/* Protection against shutdown (or other) */
static mutex_t processing_lock;
static uint32_t processing_count;
static struct mutex processing_count_lock;

/*
 * Boolean that tells if the interface is in the Connected state.
 * In this case, and only in this case, the interface can be used.
 */
static volatile bool __connected;
static struct completion connected_sync;

/*
 * The functions processing_start() and processing_end() are used to
 * prevent pre-migration suspending actions.
 * The functions can be called in multiple execution context (threads).
 *
 * Assumptions:
 * - If the frontend is suspended during processing_start, it is for a short time, until the FE gets connected.
 * - If the frontend is suspended and a shutdown operation is in progress, the ME will disappear! Therefore,
 *   we do not take care about ongoing activities. All will disappear...
 *
 */
static void processing_start(void) {

	mutex_lock(&processing_count_lock);

	if (processing_count == 0)
		mutex_lock(&processing_lock);

	processing_count++;

	mutex_unlock(&processing_count_lock);

	/* At this point, the frontend has not been closed and may be in a transient state
	 * before getting connected. We can wait until it becomes connected.
	 *
	 * If a first call to processing_start() has succeeded, subsequent calls to this function will never lead
	 * to a wait_for_completion as below since the frontend will not be able to disconnect itself (through
	 * suspend for example). The function can therefore be safely called many times (by multiple threads).
	 */

	if (!__connected)
		wait_for_completion(&connected_sync);

}

static void processing_end(void) {

	mutex_lock(&processing_count_lock);

	processing_count--;

	if (processing_count == 0)
		mutex_unlock(&processing_lock);

	mutex_unlock(&processing_count_lock);
}

void vdoga12v6nm_start(void) {
	processing_start();
}

void vdoga12v6nm_end(void) {
	processing_end();
}

bool vdoga12v6nm_is_connected(void) {
	return __connected;
}


/**
 * Allocate the ring (including the event channel) and bind to the IRQ handler.
 */
static int setup_sring(struct vbus_device *dev, bool initall) {
	int res;
	unsigned int cmd_evtchn;
	vdoga12v6nm_cmd_sring_t *cmd_sring;
	struct vbus_transaction vbt;

	if (dev->state == VbusStateConnected)
		return 0;

	DBG(VDOGA12V6NM_PREFIX "Frontend: Setup ring\n");

	/* cmd_ring */

	vdoga12v6nm.cmd_ring_ref = GRANT_INVALID_REF;

	if (initall) {
		res = vbus_alloc_evtchn(dev, &cmd_evtchn);
		BUG_ON(res);

		res = bind_evtchn_to_irq_handler(cmd_evtchn, vdoga12v6nm_cmd_interrupt, NULL, &vdoga12v6nm);
		BUG_ON(res <= 0);

		vdoga12v6nm.cmd_evtchn = cmd_evtchn;
		vdoga12v6nm.cmd_irq = res;

		cmd_sring = (vdoga12v6nm_cmd_sring_t *) get_free_vpage();
		BUG_ON(!cmd_sring);

		SHARED_RING_INIT(cmd_sring);
		FRONT_RING_INIT(&vdoga12v6nm.cmd_ring, cmd_sring, PAGE_SIZE);
	} else {
		SHARED_RING_INIT(vdoga12v6nm.cmd_ring.sring);
		FRONT_RING_INIT(&vdoga12v6nm.cmd_ring, vdoga12v6nm.cmd_ring.sring, PAGE_SIZE);
	}

	res = vbus_grant_ring(dev, phys_to_pfn(virt_to_phys_pt((uint32_t) vdoga12v6nm.cmd_ring.sring)));
	if (res < 0)
		BUG();

	vdoga12v6nm.cmd_ring_ref = res;

	/* Store the event channel and the ring ref in vbstore */

	vbus_transaction_start(&vbt);

	/* cmd_ring */

	vbus_printf(vbt, dev->nodename, "cmd_ring-ref", "%u", vdoga12v6nm.cmd_ring_ref);
	vbus_printf(vbt, dev->nodename, "cmd_ring-evtchn", "%u", vdoga12v6nm.cmd_evtchn);

	vbus_transaction_end(vbt);

	return 0;
}

/**
 * Free the ring and deallocate the proper data.
 */
static void free_sring(void) {
	/* cmd_ring */

	if (vdoga12v6nm.cmd_ring_ref != GRANT_INVALID_REF) {
		gnttab_end_foreign_access(vdoga12v6nm.cmd_ring_ref);
		free_vpage((uint32_t) vdoga12v6nm.cmd_ring.sring);

		vdoga12v6nm.cmd_ring_ref = GRANT_INVALID_REF;
		vdoga12v6nm.cmd_ring.sring = NULL;
	}

	if (vdoga12v6nm.cmd_irq)
		unbind_from_irqhandler(vdoga12v6nm.cmd_irq);

	vdoga12v6nm.cmd_irq = 0;
}

/**
 * Allocate the notification event channels and bind to the IRQ handlers.
 */
static int setup_notifications(struct vbus_device *dev) {
	int res;
	unsigned int up_evtchn, down_evtchn;
	struct vbus_transaction vbt;

	/* Up mechanical stop */

	res = vbus_alloc_evtchn(dev, &up_evtchn);
	BUG_ON(res);

	res = bind_evtchn_to_irq_handler(up_evtchn, vdoga12v6nm_up_interrupt, NULL, dev);
	BUG_ON(res <= 0);

	vdoga12v6nm.up_irq = res;

	/* Down mechanical stop */

	res = vbus_alloc_evtchn(dev, &down_evtchn);
	BUG_ON(res);

	res = bind_evtchn_to_irq_handler(down_evtchn, vdoga12v6nm_down_interrupt, NULL, dev);
	BUG_ON(res <= 0);

	vdoga12v6nm.down_irq = res;

	/* Store the event channels in vbstore */

	vbus_transaction_start(&vbt);
	vbus_printf(vbt, dev->nodename, "up-evtchn", "%u", up_evtchn);
	vbus_printf(vbt, dev->nodename, "down-evtchn", "%u", down_evtchn);
	vbus_transaction_end(vbt);

	return 0;
}

/**
 * Gnttab for the ring after migration.
 */
static void postmig_setup_sring(struct vbus_device *dev) {
	gnttab_end_foreign_access_ref(vdoga12v6nm.cmd_ring_ref);

	setup_sring(dev, false);
}

static int __vdoga12v6nm_probe(struct vbus_device *dev, const struct vbus_device_id *id) {
	vdoga12v6nm.dev = dev;

	setup_sring(dev, true);
	setup_notifications(dev);

	vdoga12v6nm_probe();

	__connected = false;

	return 0;
}

/**
 * State machine by the frontend's side.
 */
static void backend_changed(struct vbus_device *dev, enum vbus_state backend_state) {
	switch (backend_state) {
	case VbusStateReconfiguring:
		BUG_ON(__connected);

		postmig_setup_sring(dev);
		vdoga12v6nm_reconfiguring();

		mutex_unlock(&processing_lock);
		break;

	case VbusStateClosed:
		BUG_ON(__connected);

		vdoga12v6nm_closed();
		free_sring();

		/* The processing_lock is kept forever, since it has to keep all processing activities suspended.
		 * Until the ME disappears...
		 */

		break;

	case VbusStateSuspending:
		/* Suspend Step 2 */
		DBG("Got that backend %s suspending now ...\n", dev->nodename);
		mutex_lock(&processing_lock);

		__connected = false;
		reinit_completion(&connected_sync);

		vdoga12v6nm_suspend();

		break;

	case VbusStateResuming:
		/* Resume Step 2 */
		BUG_ON(__connected);

		vdoga12v6nm_resume();

		mutex_unlock(&processing_lock);
		break;

	case VbusStateConnected:
		vdoga12v6nm_connected();

		/* Now, the FE is considered as connected */
		__connected = true;

		complete(&connected_sync);
		break;

	case VbusStateUnknown:
	default:
		lprintk("%s - line %d: Unknown state %d (backend) for device %s\n", __func__, __LINE__, backend_state, dev->nodename);
		BUG();
		break;
	}
}

int __vdoga12v6nm_shutdown(struct vbus_device *dev) {

	/*
	 * Ensure all frontend processing is in a stable state.
	 * The lock will be never released once acquired.
	 * The frontend will be never be in a shutdown procedure before the end of resuming operation.
	 * It's mainly the case of a force_terminate callback which may intervene only after the frontend
	 * gets connected (not before).
	 */

	mutex_lock(&processing_lock);

	__connected = false;
	reinit_completion(&connected_sync);

	vdoga12v6nm_shutdown();

	return 0;
}

static const struct vbus_device_id vdoga12v6nm_ids[] = {
	{ VDOGA12V6NM_NAME },
	{ "" }
};

static struct vbus_driver vdoga12v6nm_drv = {
	.name			= VDOGA12V6NM_NAME,
	.ids			= vdoga12v6nm_ids,
	.probe			= __vdoga12v6nm_probe,
	.shutdown		= __vdoga12v6nm_shutdown,
	.otherend_changed	= backend_changed,
};

void vdoga12v6nm_vbus_init(void) {
	vbus_register_frontend(&vdoga12v6nm_drv);

	mutex_init(&processing_lock);
	mutex_init(&processing_count_lock);

	processing_count = 0;

	init_completion(&connected_sync);

}
