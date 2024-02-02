/*
 * Copyright (C) 2018 Baptiste Delporte <bonel@bonel.net>
 * Copyright (C) 2018 David Truan <david.truan@heig-vd.ch>
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

#ifndef VDOGA12V6NM_H
#define VDOGA12V6NM_H

#include <soo/ring.h>
#include <soo/grant_table.h>
#include <soo/vdevfront.h>

#include <ioctl.h>

#define VDOGA12V6NM_NAME	"vdoga12v6nm"
#define VDOGA12V6NM_PREFIX	"[" VDOGA12V6NM_NAME "] "

#define VDOGA12V6NM_DEV_MAJOR	120
#define VDOGA12V6NM_DEV_NAME	"/dev/vdoga12v6nm"

#define VDOGA12V6NM_PACKET_SIZE		1024

/* ioctl codes */
#define VDOGA12V6NM_ENABLE			_IOW(0x5000d08au, 1, uint32_t)
#define VDOGA12V6NM_DISABLE			_IOW(0x5000d08au, 2, uint32_t)
#define VDOGA12V6NM_SET_PERCENTAGE_SPEED	_IOW(0x5000d08au, 3, uint32_t)
#define VDOGA12V6NM_SET_ROTATION_DIRECTION	_IOW(0x5000d08au, 4, uint32_t)

#define MAX_BUF_CMDS	8

typedef struct {
	uint32_t	cmd;
	uint32_t	arg;
	bool		response_needed;
} vdoga12v6nm_request_t;

typedef struct  {
	uint32_t	cmd;
	uint32_t	arg;
	uint32_t	ret;
} vdoga12v6nm_response_t;

DEFINE_RING_TYPES(vdoga12v6nm, vdoga12v6nm_request_t, vdoga12v6nm_response_t);

typedef void(*vdoga_interrupt_t)(void);

void vdoga12v6nm_enable(void);
void vdoga12v6nm_disable(void);
void vdoga12v6nm_set_percentage_speed(uint32_t speed);
void vdoga12v6nm_set_rotation_direction(uint8_t direction);

void vdoga12v6nm_register_interrupts(vdoga_interrupt_t up, vdoga_interrupt_t down);

typedef struct {
	/* Must be the first field */
	vdevfront_t vdevfront;

	vdoga12v6nm_front_ring_t cmd_ring;

	grant_ref_t	cmd_ring_ref;
	grant_handle_t	cmd_handle;

	uint32_t	cmd_evtchn;
	uint32_t	cmd_irq;

	uint32_t	up_evtchn;
	uint32_t	up_irq;

	uint32_t	down_evtchn;
	uint32_t	down_irq;

	volatile bool connected;
	struct completion connected_sync;

} vdoga12v6nm_t;

/* ISRs associated to the ring and notifications */
irq_return_t vdoga12v6nm_cmd_interrupt(int irq, void *dev_id);
irq_return_t vdoga12v6nm_up_interrupt(int irq, void *dev_id);
irq_return_t vdoga12v6nm_down_interrupt(int irq, void *dev_id);

/*
 * Interface with the client.
 * These functions must be provided in the applicative part.
 */
void up_interrupt(void);
void down_interrupt(void);

#endif /* VDOGA12V6NM_H */
