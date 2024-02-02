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

/*
 * UPDATE 27 MARCH 2023 : This FE/BE must be re-architectured with a ring and the new model */
 */
#if 0
#define DEBUG
#endif

#include <mutex.h>
#include <heap.h>
#include <completion.h>

#include <device/driver.h>

#include <soo/evtchn.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/console.h>
#include <soo/debug.h>

#include <soo/grant_table.h>
#include <soo/vbstore.h>

#include <asm/mmu.h>
#include <memory.h>

#include <soo/dev/vweather.h>

typedef struct {
	vweather_t vweather;
} vweather_priv_t;

static struct vbus_device *vweather_dev = NULL;

static vweather_interrupt_t __update_interrupt = NULL;

vweather_data_t *vweather_get_data(void) {
	vweather_priv_t *vweather_priv = dev_get_drvdata(vweather_dev->dev);
	vweather_t *vweather = &vweather_priv->vweather;

	return (vweather_data_t *) vweather->weather_data;
}

irq_return_t vweather_update_interrupt(int irq, void *dev_id) {
	if (likely(__update_interrupt != NULL))
		(*__update_interrupt)();

	return IRQ_COMPLETED;
}


/**
 * Allocate the pages dedicated to the shared buffer.
 */
static int alloc_shared_buffer(struct vbus_device *dev) {
	vweather_t *vweather = to_vweather(dev);

	int nr_pages = DIV_ROUND_UP(VWEATHER_DATA_SIZE, PAGE_SIZE);

	/* Weather data shared buffer */

	vweather->weather_data = (char *) get_contig_free_vpages(nr_pages);
	memset(vweather->weather_data, 0, VWEATHER_DATA_SIZE);

	if (!vweather->weather_data)
		BUG();

	vweather->weather_pfn = phys_to_pfn(virt_to_phys_pt((uint32_t) vweather->weather_data));

	DBG(VWEATHER_PREFIX "Frontend: data pfn=%x\n", vweather->weather_pfn);

	return 0;
}

/**
 * Setup notification without ring.
 */
static int setup_notification(struct vbus_device *dev) {
	vweather_t *vweather = to_vweather(dev);
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

	vweather->evtchn = update_evtchn;
	vweather->irq = res;

	vbus_transaction_start(&vbt);
	vbus_printf(vbt, dev->nodename, "data_update-evtchn", "%u", vweather->evtchn);
	vbus_transaction_end(vbt);

	return 0;
}

void vweather_probe(struct vbus_device *vdev) {
	DBG0(VWEATHER_PREFIX "Frontend probe\n");

	vweather_dev = vdev;

	alloc_shared_buffer(vdev);
	setup_shared_buffer(vdev);
	setup_notification(vdev);
}

void vweather_suspend(struct vbus_device *vdev) {
	DBG0(VWEATHER_PREFIX "Frontend suspend\n");
}

void vweather_resume(struct vbus_device *vdev) {
	DBG0(VWEATHER_PREFIX "Frontend resume\n");
}

void vweather_connected(struct vbus_device *vdev) {
	vweather_priv_t *vweather_priv = dev_get_drvdata(vdev->dev);

	DBG0(VWEATHER_PREFIX "Frontend connected\n");

	/* Force the processing of pending requests, if any */
	notify_remote_via_virq(vweather_priv->vweather.irq);
}

void vweather_reconfiguring(struct vbus_device *vdev) {
	DBG0(VWEATHER_PREFIX "Frontend reconfiguring\n");

	readjust_shared_buffer(vdev);
	setup_shared_buffer(vdev);
}

void vweather_shutdown(struct vbus_device *vdev) {
	DBG0(VWEATHER_PREFIX "Frontend shutdown\n");
}

void vweather_closed(struct vbus_device *vdev) {
	DBG0(VWEATHER_PREFIX "Frontend close\n");
	free_shared_buffer(vdev);
}

void vweather_register_interrupt(vweather_interrupt_t update_interrupt) {
	__update_interrupt = update_interrupt;
}


vdrvfront_t vweatherdrv = {
	.probe = vweather_probe,
	.reconfiguring = vweather_reconfiguring,
	.shutdown = vweather_shutdown,
	.closed = vweather_closed,
	.suspend = vweather_suspend,
	.resume = vweather_resume,
	.connected = vweather_connected
};

static int vweather_init(dev_t *dev, int fdt_offset) {
	vweather_priv_t *vweather_priv;

	vweather_priv = malloc(sizeof(vweather_priv_t));
	BUG_ON(!vweather_priv);

	memset(vweather_priv, 0, sizeof(vweather_priv_t));

	dev_set_drvdata(dev, vweather_priv);

	vdevfront_init(VWEATHER_NAME, &vweatherdrv);

	return 0;
}

REGISTER_DRIVER_POSTCORE("vweather,frontend", vweather_init);
