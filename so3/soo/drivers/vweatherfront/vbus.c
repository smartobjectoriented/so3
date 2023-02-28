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

#include <soo/dev/vweather.h>

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

void vweather_start(void) {
	processing_start();
}

void vweather_end(void) {
	processing_end();
}

bool vweather_is_connected(void) {
	return __connected;
}

/**
 * Allocate the pages dedicated to the shared buffer.
 */
static int alloc_shared_buffer(void) {
	int nr_pages = DIV_ROUND_UP(VWEATHER_DATA_SIZE, PAGE_SIZE);

	/* Weather data shared buffer */

	vweather.weather_data = (char *) get_contig_free_vpages(nr_pages);
	memset(vweather.weather_data, 0, VWEATHER_DATA_SIZE);

	if (!vweather.weather_data)
		BUG();

	vweather.weather_pfn = phys_to_pfn(virt_to_phys_pt((uint32_t) vweather.weather_data));

	DBG(VWEATHER_PREFIX "Frontend: data pfn=%x\n", vweather.weather_pfn);

	return 0;
}

/**
 * Apply the pfn offset to the pages devoted to the shared buffer.
 */
static int readjust_shared_buffer(void) {
	DBG(VWEATHER_PREFIX "Frontend: pfn offset=%d\n", get_pfn_offset());

	/* Weather data shared buffer */

	if ((vweather.weather_data) && (vweather.weather_pfn)) {
		vweather.weather_pfn += get_pfn_offset();
		DBG(VWEATHER_PREFIX "Frontend: data pfn=%x\n", vweather.weather_pfn);
	}

	return 0;
}

/**
 * Store the pfn of the shared buffer.
 */
static int setup_shared_buffer(void) {
	struct vbus_transaction vbt;

	/* Data shared buffer */

	if ((vweather.weather_data) && (vweather.weather_pfn)) {
		DBG(VWEATHER_PREFIX "Frontend: data pfn=%x\n", vweather.weather_pfn);

		vbus_transaction_start(&vbt);
		vbus_printf(vbt, vweather.dev->nodename, "data-pfn", "%u", vweather.weather_pfn);
		vbus_transaction_end(vbt);
	}

	return 0;
}

/**
 * Setup notification without ring.
 */
static int setup_notification(struct vbus_device *dev) {
	int res;
	unsigned int update_evtchn;
	struct vbus_transaction vbt;

	/* Weather data update */

	res = vbus_alloc_evtchn(dev, &update_evtchn);
	if (res)
		return res;

	res = bind_evtchn_to_irq_handler(update_evtchn, vweather_update_interrupt, NULL, dev);
	if (res <= 0) {
		lprintk("%s - line %d: Binding event channel failed for device %s\n", __func__, __LINE__, dev->nodename);
		BUG();
	}

	vweather.evtchn = update_evtchn;
	vweather.irq = res;

	vbus_transaction_start(&vbt);
	vbus_printf(vbt, dev->nodename, "data_update-evtchn", "%u", vweather.evtchn);
	vbus_transaction_end(vbt);

	return 0;
}

/**
 * Free the shared buffer and deallocate the proper data.
 */
static void free_shared_buffer(void) {
	/* Weather data shared buffer */

	if ((vweather.weather_data) && (vweather.weather_pfn)) {
		free_vpage((uint32_t) vweather.weather_data);

		vweather.weather_data = NULL;
		vweather.weather_pfn = 0;
	}
}

static int __vweather_probe(struct vbus_device *dev, const struct vbus_device_id *id) {
	vweather.dev = dev;

	alloc_shared_buffer();
	setup_shared_buffer();
	setup_notification(dev);

	vweather_probe();

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
		
		readjust_shared_buffer();
		setup_shared_buffer();
		vweather_reconfiguring();

		mutex_unlock(&processing_lock);
		break;

	case VbusStateClosed:
		BUG_ON(__connected);
		
		vweather_close();
		free_shared_buffer();


		/* The processing_lock is kept forever, since it has to keep all processing activities suspended.
		 * Until the ME disappears...
		 */

		break;

	case VbusStateSuspending:
		/* Suspend Step 2 */
		mutex_lock(&processing_lock);

		__connected = false;
		reinit_completion(&connected_sync);

		vweather_suspend();

		break;

	case VbusStateResuming:
		/* Resume Step 2 */
		BUG_ON(__connected);

		vweather_resume();

		mutex_unlock(&processing_lock);
		break;

	case VbusStateConnected:
		vweather_connected();

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

int __vweather_shutdown(struct vbus_device *dev) {

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

	vweather_shutdown();

	return 0;
}

static const struct vbus_device_id vweather_ids[] = {
	{ VWEATHER_NAME },
	{ "" }
};

static struct vbus_driver vweather_drv = {
	.name = VWEATHER_NAME,
	.ids = vweather_ids,
	.probe = __vweather_probe,
	.shutdown = __vweather_shutdown,
	.otherend_changed = backend_changed,
};

void vweather_vbus_init(void) {
	vbus_register_frontend(&vweather_drv);

	mutex_init(&processing_lock);
	mutex_init(&processing_count_lock);

	processing_count = 0;

	init_completion(&connected_sync);

}
