/*
 * Copyright (C) 2020-2022 David Truan <david.truan@heig-vd.ch>
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

#ifndef VUIHANDLER_H
#define VUIHANDLER_H

#include <types.h>

#include <soo/soo.h>
#include <soo/ring.h>
#include <soo/grant_table.h>
#include <soo/vdevfront.h>

#define VUIHANDLER_NAME		"vuihandler"
#define VUIHANDLER_PREFIX	"[" VUIHANDLER_NAME "] "

#define VUIHANDLER_DEV_MAJOR	121
#define VUIHANDLER_DEV_NAME	"/dev/vuihandler"

/* Periods and delays in ms */

#define VUIHANDLER_APP_WATCH_PERIOD	10000
#define VUIHANDLER_APP_RSP_TIMEOUT	2000
#define VUIHANDLER_APP_VBSTORE_DIR	"backend/" VUIHANDLER_NAME
#define VUIHANDLER_APP_VBSTORE_NODE	"connected-app-me-spid"

/* vUIHandler packet send period */
#define VUIHANDLER_PERIOD	1000

typedef struct __attribute__((packed)) {
	int32_t		slotID;
	uint8_t		type;
	uint8_t		payload[0];
} vuihandler_pkt_t;

#define VUIHANDLER_MAX_PACKETS		8
/* Maximal size of a BT packet payload */

#define VUIHANDLER_MAX_PAYLOAD_SIZE	1024
/* Maximal size of a BT packet's data (the header is included) */

#define VUIHANDLER_MAX_PKT_SIZE		(sizeof(vuihandler_pkt_t) + VUIHANDLER_MAX_PAYLOAD_SIZE)

/* Shared buffer size */
#define VUIHANDLER_BUFFER_SIZE		(VUIHANDLER_MAX_PACKETS * VUIHANDLER_MAX_PKT_SIZE)

#define VUIHANDLER_BEACON	0
#define VUIHANDLER_DATA		1
#define VUIHANDLER_ASK_LIST	4 /* Ask for the XML ME list */
#define VUIHANDLER_SEND		5 /* Specify that the packet contains an event data to be forwarded to the ME */
#define VUIHANDLER_SELECT	6 /* Ask for the ME model */
#define VUIHANDLER_POST		7

#define VUIHANDLER_BT_PKT_HEADER_SIZE	sizeof(vuihandler_pkt_t)

#define VUIHANDLER_MAX_TX_BUF_ENTRIES	6

typedef struct {
	uint32_t		id;
	uint8_t 		type;
	size_t			size;
	uint8_t 		buf[VUIHANDLER_MAX_PAYLOAD_SIZE];
} vuihandler_tx_request_t;

/* Not used */
typedef struct {
	uint32_t		val;
} vuihandler_tx_response_t;

DEFINE_RING_TYPES(vuihandler_tx, vuihandler_tx_request_t, vuihandler_tx_response_t);

/* Not used */
typedef struct {
	uint32_t		val;
} vuihandler_rx_request_t;

typedef struct {
	uint32_t		id;
	size_t			size;
	uint8_t			type;
	uint8_t 		buf[VUIHANDLER_MAX_PAYLOAD_SIZE];
} vuihandler_rx_response_t;

DEFINE_RING_TYPES(vuihandler_rx, vuihandler_rx_request_t, vuihandler_rx_response_t);

typedef struct {
	uint64_t		spid;
	struct list_head	list;
} vuihandler_connected_app_t;


typedef struct {
	vdevfront_t vdevfront;

	vuihandler_tx_front_ring_t tx_ring;
	unsigned int tx_irq;
	grant_ref_t tx_ring_ref;
	uint32_t tx_evtchn;

    
	vuihandler_rx_front_ring_t rx_ring;
	unsigned int rx_irq;
	grant_ref_t rx_ring_ref;
	uint32_t rx_evtchn;

    	char		*tx_data;
	unsigned int	tx_pfn;

	char		*rx_data;
	unsigned int	rx_pfn;
    
} vuihandler_t;


typedef struct {
	char 		data[VUIHANDLER_MAX_PAYLOAD_SIZE];
	size_t 		size;
	uint8_t 	type;
} tx_buf_entry_t;

typedef struct {
	size_t cur_prod_idx;
	size_t cur_cons_idx;
	size_t cur_size;
	tx_buf_entry_t circ_buf[VUIHANDLER_MAX_TX_BUF_ENTRIES];

} tx_circ_buf_t;

static inline vuihandler_t *to_vuihandler(struct vbus_device *vdev) {
	vdevfront_t *vdevback = dev_get_drvdata(vdev->dev);
	return container_of(vdevback, vuihandler_t, vdevfront);
}


bool vuihandler_ready(void);

typedef void(*ui_update_spid_t)(uint8_t *);
typedef void(*ui_interrupt_t)(char *data, size_t size);
typedef void(*ui_send_model_t)(void);

/** 
 * @brief Allows to register two callbacks to the vuihandler. 
 * 
 * @param ui_send_model: callbacks to send the model. It is a function to let the ME app do whatever it
 * wants and not just pass a model char *.
 * @param ui_interrupt: callbacks to be called when receiving a VUIHANDLER_POST or VUIHANDLER_DATA packet.
 *
 */ 
void vuihandler_register_callbacks(ui_send_model_t ui_send_model,  ui_interrupt_t ui_interrupt);

void vuihandler_send(void *data, size_t size, uint8_t type);

void vuihandler_get_app_spid(uint64_t spid);

#endif /* VUIHANDLER_H */
