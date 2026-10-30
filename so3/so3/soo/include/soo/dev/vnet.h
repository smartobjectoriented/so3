/*
 * Copyright (C) 2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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
 * vnet: the packets of a capsule which carries a network (soonet), to and
 * from its agency.
 *
 * One ring for both directions: a request is a packet from the capsule, a
 * response a packet to it. Each names a port: 0 for the agency's own stack,
 * else the uid of the neighbour the packet goes to, or came from. The data of
 * entry i lies in slot i % VNET_SLOTS of the capsule's pages, granted to the
 * agency: tx pages for requests, rx pages for responses. Each side sees the
 * other's consumer index in the shared ring, so it never overruns it.
 */

#ifndef VNET_H
#define VNET_H

#include <compiler.h>
#include <soo/ring.h>
#include <soo/vdevfront.h>
#include <soo/gnttab.h>

#define VNET_NAME "vnet"
#define VNET_PREFIX "[" VNET_NAME "] "

#define VNET_SLOTS 32
#define VNET_SLOT_SIZE 2048
#define VNET_PAGES (VNET_SLOTS * VNET_SLOT_SIZE / PAGE_SIZE)
#define VNET_MTU 1500

typedef struct {
	uint64_t port;
	uint16_t len;
	uint16_t pad[3];
} vnet_request_t;

typedef struct {
	uint64_t port;
	uint16_t len;
	uint16_t pad[3];
} vnet_response_t;

DEFINE_RING_TYPES(vnet, vnet_request_t, vnet_response_t);

/* What a read returns and a write takes on /dev/vnet: the port, then the packet. */

struct vnet_packet {
	uint64_t port;
	uint8_t data[VNET_MTU];
};

typedef struct {
	/* Must be the first field */
	vdevfront_t vdevfront;

	vnet_front_ring_t ring;
	unsigned int irq;
	uint32_t evtchn;

	grant_ref_t ring_ref;
	grant_ref_t tx_ref[VNET_PAGES];
	grant_ref_t rx_ref[VNET_PAGES];

	/* Two slots per page, each page granted on its own. */

	addr_t tx_page[VNET_PAGES];
	addr_t rx_page[VNET_PAGES];
} vnet_t;

/* Where the data of a ring entry lies, in its pages. */

#define VNET_SLOT(pages, idx)                                                     \
	((void *) ((pages)[((idx) % VNET_SLOTS) / (PAGE_SIZE / VNET_SLOT_SIZE)] + \
		   ((idx) % (PAGE_SIZE / VNET_SLOT_SIZE)) * VNET_SLOT_SIZE))

#endif /* VNET_H */
