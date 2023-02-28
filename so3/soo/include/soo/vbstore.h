/*
 * Copyright (C) 2016-2018 Baptiste Delporte <bonel@bonel.net>
 * Copyright (C) 2016-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef VBSTORE_H
#define VBSTORE_H

#include <list.h>
#include <types.h>

#include <soo/vbus.h>

enum vbus_msg_type
{
	VBS_DIRECTORY,
	VBS_DIRECTORY_EXISTS,
	VBS_READ,
	VBS_WATCH,
	VBS_UNWATCH,
	VBS_TRANSACTION_END,
	VBS_WRITE,
	VBS_MKDIR,
	VBS_RM,
	VBS_WATCH_EVENT,
};
typedef enum vbus_msg_type vbus_msg_type_t;

struct msgvec {
	void *base;
	uint32_t len;
};
typedef struct msgvec msgvec_t;

struct vbus_msg
{
	struct list_head list;   /* Used to store the msg into the standby list */

	/* The next field *must* be allocated dynamically since the size of a struct completion depends
	 * on SMP enabling.
	 */
	union {
		struct completion *reply_wait;
	} u;

	uint32_t type;  	/* vbus_msg type */
	uint32_t len;   	/* Length of data following this. */

	uint32_t id;		/* Unique msg ID (32-bit circular) */
	uint32_t transactionID;	/* A non-zero value means we are in a transaction */

	struct vbus_msg *reply; /* Refer to another vbus_msg message (case of the reply) */

	/* Message content */
	char *payload;

};
typedef struct vbus_msg vbus_msg_t;

/* Inter-domain shared memory communications. */
#define VBSTORE_RING_SIZE 1024

typedef uint32_t VBSTORE_RING_IDX;

struct vbstore_domain_interface {
	char req[VBSTORE_RING_SIZE]; /* Requests to vbstore daemon. */
	char rsp[VBSTORE_RING_SIZE]; /* Replies and async watch events. */
	volatile VBSTORE_RING_IDX req_cons, req_prod, req_pvt;
	volatile VBSTORE_RING_IDX rsp_cons, rsp_prod, rsp_pvt;

	uint32_t levtchn, revtchn;
};
typedef volatile struct vbstore_domain_interface vbstore_intf_t;

struct vbs_watcher {
	struct list_head list;
	volatile vbstore_intf_t *intf;
};

struct vbs_node {
	struct list_head children; /* subtree */
	struct list_head watchers;
	struct list_head sibling;  /* at the same level */
	struct vbs_node *parent;
	char *key;
	void *value;
};

void vbstorage_agency_init(void);

struct vbus_device;

struct vbs_node *vbs_store_lookup(const char *key);
void vbs_notify_watchers(vbus_msg_t vbus_msg, struct vbs_node *node);
int vbs_store_read(const char *key, char **value, size_t size);
int vbs_store_write(const char *key, const char *value);
int vbs_store_write_notify(const char *key, const char *value, struct vbs_node **notify_node);
int vbs_store_mkdir(const char *key);
int vbs_store_mkdir_notify(const char *key, struct vbs_node **notify_node);
int vbs_store_rm(const char *key);
int vbs_store_readdir(const char *key, char *children, const size_t size_children);
int vbs_store_watch(const char *key, vbstore_intf_t *intf);
int vbs_store_unwatch(const char *key, vbstore_intf_t *intf);

void vbs_get_absolute_path(struct vbs_node *node, char *path);

void vbs_dump(void);
void vbs_dump_watches(void);

extern vbstore_intf_t *__intf;

extern struct list_head watches_rt;

extern void vbus_vbstore_init(void);

#endif /* VBSTORE_H */
