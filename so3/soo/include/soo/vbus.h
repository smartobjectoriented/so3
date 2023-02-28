/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2016, 2018 Baptiste Delporte <bonel@bonel.net>
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

#ifndef VBUS_H
#define VBUS_H

#include <completion.h>

#include <device/driver.h>

#include <soo/hypervisor.h>
#include <soo/avz.h>
#include <soo/grant_table.h>
#include <soo/dev/vbus.h>
#include <soo/vbstore.h>

/* VBUS_KEY_LENGTH as it is managed by vbstore */
#define VBS_KEY_LENGTH		50
#define VBUS_ID_SIZE		VBS_KEY_LENGTH
#define VBUS_DEV_TYPE		32

/* Register callback to watch this node. */
struct vbus_watch
{
	struct list_head list;

	/* Path being watched. */
	char *node;

	/* Callback (executed in a process context with no locks held). */
	void (*callback)(struct vbus_watch *);

	volatile bool pending;
};

struct vbus_type
{
	char *root;
	unsigned int levels;
	int (*get_bus_id)(char bus_id[VBUS_ID_SIZE], const char *nodename);

	int (*probe)(struct vbus_type *bus, const char *type, const char *dir);
	void (*otherend_changed)(struct vbus_watch *watch);

	void (*suspend)(void);
	void (*resume)(void);
};


/* A vbus device. */
struct vbus_device {
	char devicetype[VBUS_DEV_TYPE];
	char nodename[VBS_KEY_LENGTH];
	char otherend[VBS_KEY_LENGTH];
	int otherend_id;

	struct vbus_watch otherend_watch;

	dev_t *dev;

	struct vbus_type *vbus;
	struct vbus_driver *vdrv;

	/* Used by the main frontend device list */
	struct list_head list;

	enum vbus_state state;

	/* So far, only the ME use this completion struct. On the agency side,
	 * the device can not be shutdown on live.
	 */
	struct completion down;

	/* This completion lock is used for synchronizing interactions during connecting, suspending and resuming activities. */
	struct completion sync_backfront;

	int resuming;

	bool realtime;  /* Tell if this device is a RT device */
};


/* A vbus driver. */
struct vbus_driver {
	char *name;

	char devicetype[32];

	void (*probe)(struct vbus_device *dev);
	void (*otherend_changed)(struct vbus_device *dev, enum vbus_state backend_state);

	void (*shutdown)(struct vbus_device *dev);

	void (*suspend)(struct vbus_device *dev);
	void (*resume)(struct vbus_device *dev);
	void (*resumed)(struct vbus_device *dev);

	/* Used as an entry of the main vbus driver list */
	struct list_head list;

	void (*read_otherend_details)(struct vbus_device *dev);
};


void vbus_register_frontend(struct vbus_driver *drv);
void vbus_unregister_driver(struct vbus_driver *drv);

struct vbus_transaction
{
	u32 id; /* Unique non-zereo value to identify a transaction */
};

/* Nil transaction ID. */
#define VBT_NIL ((struct vbus_transaction) { 0 })

char **vbus_directory(struct vbus_transaction t, const char *dir, const char *node, unsigned int *num);
int vbus_directory_exists(struct vbus_transaction t, const char *dir, const char *node);
void *vbus_read(struct vbus_transaction t, const char *dir, const char *node, unsigned int *len);
void vbus_write(struct vbus_transaction t, const char *dir, const char *node, const char *string);
void vbus_mkdir(struct vbus_transaction t, const char *dir, const char *node);
int vbus_exists(struct vbus_transaction t, const char *dir, const char *node);
void vbus_rm(struct vbus_transaction t, const char *dir, const char *node);

void vbus_transaction_start(struct vbus_transaction *t);
void vbus_transaction_end(struct vbus_transaction t);

/* Single read and scanf: returns -errno or num scanned if > 0. */
int vbus_scanf(struct vbus_transaction t, const char *dir, const char *node, const char *fmt, ...)
	__attribute__((format(scanf, 4, 5)));

/* Single printf and write: returns -errno or 0. */
void vbus_printf(struct vbus_transaction t, const char *dir, const char *node, const char *fmt, ...)
	__attribute__((format(printf, 4, 5)));

/* Generic read function: NULL-terminated triples of name,
 * sprintf-style type string, and pointer. Returns 0 or errno.*/
bool vbus_gather(struct vbus_transaction t, const char *dir, ...);

void free_otherend_watch(struct vbus_device *dev, bool with_vbus);

extern void vbstore_init(void);
extern void vbstore_me_init(void);

bool is_vbstore_populated(void);

void register_vbus_watch(struct vbus_watch *watch);
void unregister_vbus_watch_without_vbus(struct vbus_watch *watch);
void unregister_vbus_watch(struct vbus_watch *watch);

void frontend_for_each(void *data, int (*fn)(struct vbus_device *, void *));

int vbus_dev_remove(struct vbus_device *dev);

extern unsigned int dc_evtchn;

void add_new_dev(struct vbus_device *dev);
extern int vbus_dev_probe(struct vbus_device *dev);

extern int vbus_dev_probe_frontend(dev_t *_dev);
extern int vbus_dev_remove(struct vbus_device *dev);
extern int vbus_register_driver_common(struct vbus_driver *drv);

extern int vbus_probe_devices(struct vbus_type *bus);

extern void vbus_dev_changed(const char *node, char *type, struct vbus_type *bus, const char *compat);

extern void vbus_dev_shutdown(struct vbus_device *dev);

extern void vbus_otherend_changed(struct vbus_watch *watch);

extern void vbus_read_otherend_details(struct vbus_device *vdev, char *id_node, char *path_node);

/* Prepare for domain suspend: then resume or cancel the suspend. */
int vbus_suspend_devices(unsigned int domID);
int vbus_resume_devices(unsigned int domID);

int vdev_probe(char *node, const char *compat);

void vbus_probe_frontend_init(void);

#define VBUS_IS_ERR_READ(str) ({			\
	if (!IS_ERR(str) && strlen(str) == 0) {		\
		kfree(str);				\
		str = ERR_PTR(-ERANGE);			\
	}						\
	IS_ERR(str);					\
})

#define VBUS_EXIST_ERR(err) ((err) == -ENOENT || (err) == -ERANGE)

void vbus_watch_path(struct vbus_device *dev, char *path, struct vbus_watch *watch, void (*callback)(struct vbus_watch *));
void vbus_watch_pathfmt(struct vbus_device *dev, struct vbus_watch *watch, void (*callback)(struct vbus_watch *), const char *pathfmt, ...)
	__attribute__ ((format (printf, 4, 5)));

int vbus_grant_ring(struct vbus_device *dev, unsigned long ring_mfn);
int vbus_map_ring_valloc(struct vbus_device *dev, int gnt_ref, void **vaddr);
int vbus_map_ring(struct vbus_device *dev, int gnt_ref, grant_handle_t *handle, void *vaddr);

void vbus_alloc_evtchn(struct vbus_device *dev, uint32_t *port);
void vbus_bind_evtchn(struct vbus_device *dev, uint32_t remote_port, uint32_t *port);
void vbus_free_evtchn(struct vbus_device *dev, uint32_t port);

enum vbus_state vbus_read_driver_state(const char *path);
bool vbus_read_driver_realtime(const char *path);

const char *vbus_strstate(enum vbus_state state);
int vbus_dev_is_online(struct vbus_device *dev);

int vbus_frontend_closed(struct vbus_device *dev);
int vbus_frontend_terminated(struct vbus_device *dev);
void vbus_frontend_suspended(struct vbus_device *dev);
void vbus_frontend_resumed(struct vbus_device *dev);

void remove_vbstore_entries(void);

void vbuswatch_thread_sync(void);
int get_vbstore_evtchn(void);

void ping_remote_domain(int domID, void (*ping_callback)(void));

void vbs_suspend(void);
void vbs_resume(void);

void presetup_register_watch(struct vbus_watch *watch);
void presetup_unregister_watch(struct vbus_watch *watch);

void vbstore_init_dev_populate(void);
void vbstore_devices_populate(void);
void vbstore_trigger_dev_probe(void);

int vbstore_uart_remove(unsigned int domID);

void postmig_setup(void);
int gnttab_remove(bool with_vbus);

void device_shutdown(void);
void remove_devices(void);

void vbus_init(void);

void postmig_vbstore_setup(struct DOMCALL_sync_domain_interactions_args *args);

#ifdef DEBUG
#undef DBG
#define DBG(fmt, ...) \
    do { \
        printk("%s:%i > "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while(0)
#else
#define DBG(fmt, ...)
#endif

#endif /* VBUS_H */
