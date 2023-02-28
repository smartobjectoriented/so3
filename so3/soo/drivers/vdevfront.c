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

#if 0
#define DEBUG
#endif

#include <mutex.h>

#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/console.h>
#include <soo/debug.h>
#include <soo/vdevfront.h>


/* vdevfront_processing_start() and vdevfront_processing_end() will prevent against
 * suspending/closing actions.
 *
 * The functions can be called in multiple execution context (threads).
 *
 * Assumptions:
 * - If the frontend is suspended during processing_start, it is for a short time, until the FE gets connected.
 * - If the frontend is suspended and a shutdown operation is in progress, the ME will disappear! Therefore,
 *   we do not take care about ongoing activities. All will disappear...
 *
 */
bool vdevfront_processing_begin(struct vbus_device *vdev) {
	void *priv = dev_get_drvdata(vdev->dev);
	vdevfront_t *vdevfront = (vdevfront_t *) priv;

	/* Could be still being initialized... */
	if (vdev->state != VbusStateConnected)
		wait_for_completion(&vdevfront->sync);

	if (atomic_read(&vdevfront->processing_count) == 0) {
		atomic_inc(&vdevfront->processing_count);

		mutex_lock(&vdevfront->processing_lock);

		if (vdev->state != VbusStateConnected)
			wait_for_completion(&vdevfront->sync);


		BUG_ON(vdev->state != VbusStateConnected);
	} else
		atomic_inc(&vdevfront->processing_count);

	return true;
}

/*
 * Finish a processing section against suspend/close prevention
 */
void vdevfront_processing_end(struct vbus_device *vdev) {
	void *priv = dev_get_drvdata(vdev->dev);
	vdevfront_t *vdevfront = (vdevfront_t *) priv;

	atomic_dec(&vdevfront->processing_count);

	if (atomic_read(&vdevfront->processing_count) == 0)
		mutex_unlock(&vdevfront->processing_lock);

}

/**
 * Entry point to this code when a new device is created.  Allocate the basic
 * structures and the ring buffer for communication with the backend, and
 * inform the backend of the appropriate details for those.  Switch to
 * Initialised state.
 *
 */
static void __probe(struct vbus_device *vdev) {
	void *priv = dev_get_drvdata(vdev->dev);
	vdevfront_t *vdevfront = (vdevfront_t *) priv;
	vdrvfront_t *vdrvfront = to_vdrvfront(vdev);

	DBG("%s: SOO dummy frontend driver for testing\n", __func__);

	vdrvfront->probe(vdev);

	atomic_set(&vdevfront->processing_count, 0);

	mutex_init(&vdevfront->processing_lock);

	init_completion(&vdevfront->sync);
}

/**
 * State machine by the frontend's side.
 */
static void __otherend_changed(struct vbus_device *vdev, enum vbus_state backend_state) {
	void *priv = dev_get_drvdata(vdev->dev);
	vdevfront_t *vdevfront = (vdevfront_t *) priv;
	vdrvfront_t *vdrvfront = to_vdrvfront(vdev);

	DBG("SOO vdummy frontend, backend %s changed its state to %d.\n", vdev->nodename, backend_state);

	switch (backend_state) {

	case VbusStateReconfiguring:
		BUG_ON(vdev->state == VbusStateConnected);
		
		BUG_ON(!vdrvfront->reconfiguring);
		vdrvfront->reconfiguring(vdev);

		mutex_unlock(&vdevfront->processing_lock);
		break;

	case VbusStateClosed:
		BUG_ON(vdev->state == VbusStateConnected);
		BUG_ON(!vdrvfront->closed);

		vdrvfront->closed(vdev);

		/* The processing_lock is kept forever, since it has to keep all processing activities suspended.
		 * Until the ME disappears...
		 */

		break;

	case VbusStateSuspending:
		/* Suspend Step 2 */
		DBG("Got that backend %s suspending now ...\n", dev->nodename);
		mutex_lock(&vdevfront->processing_lock);

		reinit_completion(&vdevfront->sync);
		BUG_ON(!vdrvfront->suspend);

		vdrvfront->suspend(vdev);
		break;

	case VbusStateResuming:
		/* Resume Step 2 */
		DBG("Got that backend %s resuming now.....\n", dev->nodename);

		BUG_ON(vdev->state == VbusStateConnected);
		BUG_ON(!vdrvfront->resume);

		vdrvfront->resume(vdev);

		mutex_unlock(&vdevfront->processing_lock);
		break;

	case VbusStateConnected:
		vdrvfront->connected(vdev);

		/* Now, the FE is considered as connected */

		complete(&vdevfront->sync);
		break;

	case VbusStateUnknown:
	default:
		lprintk("%s - line %d: Unknown state %d (backend) for device %s\n", __func__, __LINE__, backend_state, vdev->nodename);
		BUG();
	}
}

static void __shutdown(struct vbus_device *vdev) {
	vdevfront_t *vdevfront = dev_get_drvdata(vdev->dev);
	vdrvfront_t *vdrvfront = to_vdrvfront(vdev);

	/*
	 * Ensure all frontend processing is in a stable state.
	 * The lock will be never released once acquired.
	 * The frontend will be never be in a shutdown procedure before the end of resuming operation.
	 * It's mainly the case of a force_terminate callback which may intervene only after the frontend
	 * gets connected (not before).
	 */

	mutex_lock(&vdevfront->processing_lock);

	reinit_completion(&vdevfront->sync);

	vdrvfront->shutdown(vdev);
}


void vdevfront_init(char *name, vdrvfront_t *vfrontdrv) {

	vfrontdrv->vdrv.name = name;
	strcpy(vfrontdrv->vdrv.devicetype, name);

	vfrontdrv->vdrv.probe = __probe;

	vfrontdrv->vdrv.shutdown = __shutdown;

	vfrontdrv->vdrv.otherend_changed = __otherend_changed;

	vbus_register_frontend(&vfrontdrv->vdrv);

}
