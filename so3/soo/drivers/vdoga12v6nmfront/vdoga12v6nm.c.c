/*
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

#include <mutex.h>
#include <heap.h>
#include <completion.h>

#include <device/driver.h>

#include <soo/evtchn.h>
#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/console.h>
#include <soo/debug.h>

#include <soo/dev/vdoga12v6nm.h>
#include <soo/vdevfront.h>

vdoga12v6nm_t *__vdoga12v6nm;

static completion_t cmd_completion;
static int cmd_ret = 0;

static vdoga_interrupt_t __up_interrupt = NULL, __down_interrupt = NULL;

/**
 * Process pending responses in the cmd ring.
 */
static void process_pending_cmd_rsp(void) {
	RING_IDX i, rp;
	vdoga12v6nm_cmd_response_t *ring_rsp;

	rp = __vdoga12v6nm->cmd_ring.sring->rsp_prod;
	dmb();

	for (i = __vdoga12v6nm->cmd_ring.sring->rsp_cons; i != rp; i++) {
		ring_rsp = RING_GET_RESPONSE(&__vdoga12v6nm->cmd_ring, i);

		DBG("Ret=%d\n", ring_rsp->ret);
		cmd_ret = ring_rsp->ret;

		complete(&cmd_completion);
	}

	__vdoga12v6nm->cmd_ring.sring->rsp_cons = i;
}

/**
 * Process a command return value coming from the backend.
 */
irq_return_t vdoga12v6nm_cmd_interrupt(int irq, void *dev_id) {

	if (!vdoga12v6nm_is_connected())
		return IRQ_COMPLETED;

	process_pending_cmd_rsp();

	return IRQ_COMPLETED;
}

/**
 * Process an up mechanical stop event.
 */
irq_return_t vdoga12v6nm_up_interrupt(int irq, void *dev_id) {

	if (__up_interrupt)
		(*__up_interrupt)();

	return IRQ_COMPLETED;
}

/**
 * Process a down mechanical stop event.
 */
irq_return_t vdoga12v6nm_down_interrupt(int irq, void *dev_id) {

	if (__down_interrupt)
		(*__down_interrupt)();

	return IRQ_COMPLETED;
}

/**
 * Generate a command request.
 */
static void do_cmd(uint32_t cmd, uint32_t arg) {
	vdoga12v6nm_cmd_request_t *ring_req;

	vdoga12v6nm_start();

	DBG(VDOGA12V6NM_PREFIX "0x%08x\n", cmd);

	switch (cmd) {
	case VDOGA12V6NM_ENABLE:
	case VDOGA12V6NM_DISABLE:
	case VDOGA12V6NM_SET_PERCENTAGE_SPEED:
	case VDOGA12V6NM_SET_ROTATION_DIRECTION:
		break;

	default:
		BUG();
	}

	ring_req = RING_GET_REQUEST(&__vdoga12v6nm->cmd_ring, __vdoga12v6nm->cmd_ring.req_prod_pvt);

	ring_req->cmd = cmd;
	ring_req->arg = arg;

	dmb();

	__vdoga12v6nm->cmd_ring.req_prod_pvt++;

	RING_PUSH_REQUESTS(&__vdoga12v6nm->cmd_ring);

	notify_remote_via_virq(__vdoga12v6nm->cmd_irq);

	vdoga12v6nm_end();
}

/**
 * Enable the motor.
 */
void vdoga12v6nm_enable(void) {
	do_cmd(VDOGA12V6NM_ENABLE, 0);

	/* Wait for the command to be processed by the agency's side */
	wait_for_completion(&cmd_completion);
}

/**
 * Disable the motor.
 */
void vdoga12v6nm_disable(void) {
	do_cmd(VDOGA12V6NM_DISABLE, 0);

	/* Wait for the command to be processed by the agency's side */
	wait_for_completion(&cmd_completion);
}

/**
 * Set the motor's speed, expressed in percentage.
 */
void vdoga12v6nm_set_percentage_speed(uint32_t speed) {
	do_cmd(VDOGA12V6NM_SET_PERCENTAGE_SPEED, speed);

	/* Wait for the command to be processed by the agency's side */
	wait_for_completion(&cmd_completion);
}

/**
 * Set the motor's rotation direction.
 */
void vdoga12v6nm_set_rotation_direction(uint8_t direction) {
	do_cmd(VDOGA12V6NM_SET_ROTATION_DIRECTION, direction);

	/* Wait for the command to be processed by the agency's side */
	wait_for_completion(&cmd_completion);
}

void vdoga12v6nm_probe(struct vbus_device *vdev) {
	DBG0(VDOGA12V6NM_PREFIX " Frontend probe\n");
	unsigned int evtchn;
	vdoga12v6nm_cmd_sring_t *sring;
	struct vbus_transaction vbt;
	vdoga12v6nm_t *vdoga12v6nm;


	if (vdev->state != VbusStateConnected) 
		return;

	vdoga12v6nm = malloc(sizeof(vdoga12v6nm_t));
	BUG_ON(!vdoga12v6nm);
	memset(vdoga12v6nm, 0, sizeof(vdoga12v6nm_t));
	/* Save the pointer in the static global pointer to access it from anywhere. */
	__vdoga12v6nm = vdoga12v6nm;
	dev_set_drvdata(vdev->dev, &vdoga12v6nm->vdevfront);

	vdoga12v6nm->ring_ref = GRANT_INVALID_REF;

	
}

void vdoga12v6nm_reconfiguring(struct vbus_device *vdev) {
	DBG0(VDOGA12V6NM_PREFIX " Frontend reconfiguring\n");
}

void vdoga12v6nm_shutdown(struct vbus_device *vdev) {
	DBG0(VDOGA12V6NM_PREFIX " Frontend shutdown\n");
}

void vdoga12v6nm_closed(struct vbus_device *vdev) {
	DBG0(VDOGA12V6NM_PREFIX " Frontend close\n");
}

void vdoga12v6nm_suspend(struct vbus_device *vdev) {
	DBG0(VDOGA12V6NM_PREFIX " Frontend suspend\n");
}

void vdoga12v6nm_resume(struct vbus_device *vdev) {
	DBG0(VDOGA12V6NM_PREFIX " Frontend resume\n");

	process_pending_cmd_rsp();
}

void vdoga12v6nm_connected(struct vbus_device *vdev) {
	DBG0(VDOGA12V6NM_PREFIX " Frontend connected\n");
}

void vdoga12v6nm_register_interrupts(vdoga_interrupt_t up, vdoga_interrupt_t down) {

	__up_interrupt = up;
	__down_interrupt = down;
}

vdrvfront_t vdoga12v6nmdrv = {
	.probe = vdoga12v6nm_probe,
	.reconfiguring = vdoga12v6nm_reconfiguring,
	.shutdown = vdoga12v6nm_shutdown,
	.closed = vdoga12v6nm_closed,
	.suspend = vdoga12v6nm_suspend,
	.resume = vdoga12v6nm_resume,
	.connected = vdoga12v6nm_connected
};

int vdoga12v6nm_init(dev_t *dev) {
	init_completion(&cmd_completion);

	vdevfront_init(VDOGA12V6NM_NAME, &vdoga12v6nmdrv);

	return 0;
}

REGISTER_DRIVER_POSTCORE("vdoga12v6nm,frontend", vdoga12v6nm_init);
