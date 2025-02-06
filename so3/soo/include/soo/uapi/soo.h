/*
 * Copyright (C) 2014-2025 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef UAPI_SOO_H
#define UAPI_SOO_H

#ifndef __ASSEMBLY__

/* This signature is used to check the coherency of the ME image, after a migration
 * or a restoration for example.
 */
#define SOO_ME_SIGNATURE "SooZ"

#define MAX_ME_DOMAINS 5

/* We include the (non-RT & RT) agency domain */
#define MAX_DOMAINS (2 + MAX_ME_DOMAINS)
#endif /* __ASSEMBLY__ */

#define AGENCY_CPU 0
#define AGENCY_RT_CPU 1

#ifndef __ASSEMBLY__

#include <asm/atomic.h>

#ifndef DOMID_T
#define DOMID_T
typedef uint16_t domid_t;
typedef unsigned long addr_t;
#endif

typedef uint32_t grant_ref_t;

/* Grant table management */
#define GRANT_INVALID_REF 0

/* List of grant table commands which are processed by the hypervisor */

#define GNTTAB_grant_page 1
#define GNTTAB_revoke_page 2
#define GNTTAB_map_page 3
#define GNTTAB_unmap_page 4

struct gnttab_op {
	/* Command to be processed in the hypercall */
	uint32_t cmd;

	/* Peer domain */
	domid_t domid;

	/* pfn to be granted or pfn associated to an existing ref */
	addr_t pfn;

	/* Grant reference */
	grant_ref_t ref;
};
typedef struct gnttab_op gnttab_op_t;

void do_gnttab(gnttab_op_t *args);

/* Event channel management */

#define ECS_FREE 0 /* Channel is available for use.                  */
#define ECS_RESERVED 1 /* Channel is reserved.                           */
#define ECS_UNBOUND 2 /* Channel is waiting to bind to a remote domain. */
#define ECS_INTERDOMAIN 3 /* Channel is bound to another domain.            */
#define ECS_VIRQ 4 /* Channel is bound to a virtual IRQ line.        */

#define EVTCHNSTAT_closed 0 /* Channel is not in use.                 */
#define EVTCHNSTAT_unbound 1 /* Channel is waiting interdom connection.*/
#define EVTCHNSTAT_interdomain 2 /* Channel is connected to remote domain. */
#define EVTCHNSTAT_virq 3 /* Channel is bound to a virtual IRQ line */
/*
 * EVTCHNOP_alloc_unbound: Allocate a evtchn in domain <dom> and mark as
 * accepting interdomain bindings from domain <remote_dom>. A fresh evtchn
 * is allocated in <dom> and returned as <evtchn>.
 * NOTES:
 *  1. If the caller is unprivileged then <dom> must be DOMID_SELF.
 *  2. <rdom> may be DOMID_SELF, allowing loopback connections.
 */
#define EVTCHNOP_alloc_unbound 6
struct evtchn_alloc_unbound {
	/* IN parameters */
	domid_t dom, remote_dom;
	/* OUT parameters */
	uint32_t evtchn;
	uint32_t use;
};
typedef struct evtchn_alloc_unbound evtchn_alloc_unbound_t;

/*
 * EVTCHNOP_bind_interdomain: Construct an interdomain event channel between
 * the calling domain and <remote_dom>. <remote_dom,remote_evtchn> must identify
 * a evtchn that is unbound and marked as accepting bindings from the calling
 * domain. A fresh evtchn is allocated in the calling domain and returned as
 * <local_evtchn>.
 * NOTES:
 *  2. <remote_dom> may be DOMID_SELF, allowing loopback connections.
 */
#define EVTCHNOP_bind_interdomain 0
#define EVTCHNOP_bind_existing_interdomain 7
#define EVTCHNOP_unbind_domain 12

struct evtchn_bind_interdomain {
	/* IN parameters. */
	domid_t remote_dom;
	uint32_t remote_evtchn;
	uint32_t use;
	/* OUT parameters. */
	uint32_t local_evtchn;
};
typedef struct evtchn_bind_interdomain evtchn_bind_interdomain_t;

#define EVTCHNOP_bind_virq 1
struct evtchn_bind_virq {
	/* IN parameters. */
	uint32_t virq;
	/* OUT parameters. */
	uint32_t evtchn;
};
typedef struct evtchn_bind_virq evtchn_bind_virq_t;

/*
 * EVTCHNOP_close: Close a local event channel <evtchn>. If the channel is
 * interdomain then the remote end is placed in the unbound state
 * (EVTCHNSTAT_unbound), awaiting a new connection.
 */
#define EVTCHNOP_close 3
struct evtchn_close {
	/* IN parameters. */
	uint32_t evtchn;
};
typedef struct evtchn_close evtchn_close_t;

/*
 * EVTCHNOP_send: Send an event to the remote end of the channel whose local
 * endpoint is <evtchn>.
 */
#define EVTCHNOP_send 4
struct evtchn_send {
	/* IN parameters. */
	uint32_t evtchn;
};
typedef struct evtchn_send evtchn_send_t;

struct evtchn_op {
	uint32_t cmd; /* EVTCHNOP_* */
	union {
		struct evtchn_alloc_unbound alloc_unbound;
		struct evtchn_bind_interdomain bind_interdomain;
		struct evtchn_bind_virq bind_virq;
		struct evtchn_close close;
		struct evtchn_send send;
	} u;
};
typedef struct evtchn_op evtchn_op_t;

/* Domain control management */
/*
 * There are two main scheduling policies: the one used for normal (standard) ME, and
 * a second one used for realtime ME.
 */
#define AVZ_SCHEDULER_FLIP 0
#define AVZ_SCHEDULER_RT 1

#define DOMCTL_pauseME 1
#define DOMCTL_unpauseME 2
#define DOMCTL_get_AVZ_shared 3

struct domctl {
	uint32_t cmd;
	domid_t domain;
	addr_t avz_shared_paddr;
};
typedef struct domctl domctl_t;

/*
 * ME states:
 * - ME_state_booting:		ME is currently booting...
 * - ME_state_living:		ME is full-functional and activated (all frontend devices are consistent)
 * - ME_state_suspended:	ME is suspended before migrating. This state is maintained for the resident ME instance
 * - ME_state_hibernate:	ME is in a state of hibernate snapshot
 * - ME_state_resuming:         ME ready to perform resuming (after recovering)
 * - ME_state_awakened:         ME is just being awakened
 * - ME_state_terminated:	ME has been terminated (by a force_terminate)
 * - ME_state_dead:		ME does not exist
 */
typedef enum {
	ME_state_booting,
	ME_state_living,
	ME_state_suspended,
	ME_state_hibernate,
	ME_state_resuming,
	ME_state_awakened,
	ME_state_killed,
	ME_state_terminated,
	ME_state_dead
} ME_state_t;

/* Keep information about slot availability
 * FREE:	the slot is available (no ME)
 * BUSY:	the slot is allocated a ME
 */
typedef enum { ME_SLOT_FREE, ME_SLOT_BUSY } ME_slotState_t;

/* ME ID related information */
#define ME_NAME_SIZE 40
#define ME_SHORTDESC_SIZE 1024

/*
 * Definition of ME ID information used by functions which need
 * to get a list of running MEs with their information.
 */
typedef struct {
	uint32_t slotID;
	ME_state_t state;

	uint64_t spid;

	char name[ME_NAME_SIZE];
	char shortdesc[ME_SHORTDESC_SIZE];
} ME_id_t;

struct work_struct;
struct semaphore;

/*
 * Directcomm event management
 */
typedef enum {
	DC_NO_EVENT,
	DC_PRE_SUSPEND,
	DC_SUSPEND,
	DC_RESUME,
	DC_FORCE_TERMINATE,
	DC_POST_ACTIVATE,
	DC_TRIGGER_DEV_PROBE,
	DC_TRIGGER_LOCAL_COOPERATION,

	DC_EVENT_MAX /* Used to determine the number of DC events */
} dc_event_t;

/*
 * Callback function associated to dc_event.
 */
typedef void(dc_event_fn_t)(dc_event_t dc_event);

extern atomic_t dc_outgoing_domID[DC_EVENT_MAX];
extern atomic_t dc_incoming_domID[DC_EVENT_MAX];

/*
 * IOCTL commands for migration.
 * This part is shared between the kernel and user spaces.
 */

/*
 * IOCTL codes
 */
#define AGENCY_IOCTL_INIT_MIGRATION _IOWR('S', 0, agency_ioctl_args_t)
#define AGENCY_IOCTL_GET_ME_FREE_SLOT _IOWR('S', 1, agency_ioctl_args_t)
#define AGENCY_IOCTL_READ_SNAPSHOT _IOWR('S', 2, agency_ioctl_args_t)
#define AGENCY_IOCTL_WRITE_SNAPSHOT _IOW('S', 3, agency_ioctl_args_t)
#define AGENCY_IOCTL_FINAL_MIGRATION _IOW('S', 4, agency_ioctl_args_t)
#define AGENCY_IOCTL_FORCE_TERMINATE _IOW('S', 5, agency_ioctl_args_t)
#define AGENCY_IOCTL_INJECT_ME _IOWR('S', 6, agency_ioctl_args_t)
#define AGENCY_IOCTL_GET_ME_ID _IOWR('S', 7, agency_ioctl_args_t)
#define AGENCY_IOCTL_GET_ME_ID_ARRAY _IOR('S', 11, agency_ioctl_args_t)
#define AGENCY_IOCTL_BLACKLIST_SOO _IOW('S', 12, agency_ioctl_args_t)

#define SOO_NAME_SIZE 16

/*
 * ME descriptor
 *
 * WARNING !! Be careful when modifying this structure. It *MUST* be aligned with
 * the same structure used in the ME.
 */
typedef struct {
	unsigned int slotID;
	uint64_t spid;

	ME_state_t state;

	unsigned int size; /* Size of the ME with the struct dom_context size */
	unsigned int dc_evtchn;

	unsigned int vbstore_revtchn, vbstore_levtchn;
	addr_t vbstore_pfn;

	void (*resume_fn)(void);

} ME_desc_t;

/*
 * Agency descriptor
 */
typedef struct {
	/*
	 * SOO agencyUID unique 64-bit ID - Allowing to identify a SOO device.
	 * agencyUID 0 is NOT valid.
	 */

	uint64_t agencyUID; /* Agency UID */

	/* Event channels used for directcomm channel between agency and agency-RT or ME */
	unsigned int dc_evtchn[MAX_DOMAINS];

	/* Event channels used by vbstore */
	unsigned int vbstore_evtchn[MAX_DOMAINS];

	/* Local agency event channel for vbstore */
	uint32_t vbstore_levtchn;

} agency_desc_t;

/*
 * SOO agency & ME descriptor - This structure is used in the shared info page of the agency or ME domain.
 */

typedef struct {
	union {
		agency_desc_t agency;
		ME_desc_t ME;
	} u;
} dom_desc_t;

/* struct agency_ioctl_args used in IOCTLs */
typedef struct agency_ioctl_args {
	void *buffer; /* IN/OUT */
	int slotID;
	long value; /* IN/OUT */
} agency_ioctl_args_t;

#define NSECS 1000000000ull
#define SECONDS(_s) ((u64)((_s) * 1000000000ull))
#define MILLISECS(_ms) ((u64)((_ms) * 1000000ull))
#define MICROSECS(_us) ((u64)((_us) * 1000ull))

#define VBUS_TASK_PRIO 50

/*
 * The priority of the Directcomm thread must be higher than the priority of the SDIO
 * thread to make the Directcomm thread process a DC event and release it before any new
 * request by the SDIO's side thus avoiding a deadlock.
 */
#define DC_ISR_TASK_PRIO 55

#define SDIO_IRQ_TASK_PRIO 50
#define SDHCI_FINISH_TASK_PRIO 50

#ifndef __ASSEMBLY__

extern volatile bool __cobalt_ready;

void rtdm_register_dc_event_callback(dc_event_t dc_event,
				     dc_event_fn_t *callback);

#endif /* __ASSEMBLY__ */

/* Console management */

#define CONSOLE_IO_KEYHANDLER 0
#define CONSOLE_IO_PRINTCH 1
#define CONSOLE_IO_PRINTSTR 2

#define CONSOLE_STR_MAX_LEN 128

typedef struct {
	int cmd;
	union {
		char c;
		char str[CONSOLE_STR_MAX_LEN];
	} u;
} console_t;

/*
 * SOO hypercall management
 */

/* AVZ hypercalls devoted to SOO */
#define AVZ_ME_READ_SNAPSHOT 6
#define AVZ_ME_WRITE_SNAPSHOT 7
#define AVZ_INJECT_ME 9
#define AVZ_KILL_ME 10
#define AVZ_DC_EVENT_SET 11
#define AVZ_GET_ME_STATE 13
#define AVZ_SET_ME_STATE 14
#define AVZ_GET_DOM_DESC 16
#define AVZ_EVENT_CHANNEL_OP 17
#define AVZ_CONSOLE_IO_OP 18
#define AVZ_DOMAIN_CONTROL_OP 19
#define AVZ_GRANT_TABLE_OP 20

/* AVZ_EVENT_CHANNEL_OP */
typedef struct {
	evtchn_op_t evtchn_op;
} avz_evtchn_t;

/* AVZ_INJECT_ME */
typedef struct {
	void *itb_paddr;
	int slotID;
} avz_inject_me_t;

/* AVZ_DC_EVENT_SET */
typedef struct {
	unsigned int domID;
	dc_event_t dc_event;
	int state;
} avz_dc_event_t;

/* AVZ_GET_ME_STATE */
/* AVZ_SET_ME_STATE */
typedef struct {
	uint32_t slotID;
	int state;
} avz_me_state_t;

/* AVZ_GET_DOM_DESC */
typedef struct {
	uint32_t slotID;
	dom_desc_t dom_desc;
} avz_dom_desc_t;

/* AVZ_GET_ME_FREE_SLOT */
typedef struct {
	int slotID;
	int size;
} avz_free_slot_t;

/* AVZ_MIG_INIT */
typedef struct {
	int slotID;
} avz_mig_init_t;

/* AVZ_READ_SNAPSHOT */
/* AVZ_WRITE_SNAPSHOT */
typedef struct {
	void *snapshot_paddr;
	uint32_t slotID;
	int size;
} avz_snapshot_t;

/* AVZ_MIG_FINAL */
typedef struct {
	uint32_t slotID;
} avz_mig_final_t;

/* AVZ_KILL_ME */
typedef struct {
	uint32_t slotID;
} avz_kill_me_t;

/* AVZ_CONSOLE_IO_OP */
typedef struct {
	console_t console;
} avz_console_io_t;

/* AVZ_DOMAIN_CONTROL_OP */
typedef struct {
	domctl_t domctl;
} avz_domctl_t;

/* AVZ_GRANT_TABLE_OP */
typedef struct {
	gnttab_op_t gnttab_op;
} avz_gnttab_t;

/*
 * AVZ hypercall argument
 */
typedef struct {
	int cmd;
	union {
		avz_evtchn_t avz_evtchn;
		avz_inject_me_t avz_inject_me_args;
		avz_dc_event_t avz_dc_event_args;
		avz_me_state_t avz_me_state_args;
		avz_dom_desc_t avz_dom_desc_args;
		avz_free_slot_t avz_free_slot_args;
		avz_mig_init_t avz_mig_init_args;
		avz_snapshot_t avz_snapshot_args;
		avz_mig_final_t avz_mig_final_args;
		avz_kill_me_t avz_kill_me_args;
		avz_console_io_t avz_console_io_args;
		avz_domctl_t avz_domctl_args;
		avz_gnttab_t avz_gnttab_args;
	} u;
} avz_hyp_t;

typedef struct {
	void *val;
} pre_suspend_args_t;

typedef struct {
	void *val;
} pre_resume_args_t;

typedef struct {
	void *val;
} post_activate_args_t;

/*
 * Further agency ctl commands that may be used by MEs.
 * !! WARNING !! The ME must implement the same definitions.
 */

#define AG_INJECT_ME 0x11
#define AG_KILL_ME 0x12

typedef struct {
	char soo_name[SOO_NAME_SIZE];
} soo_name_args_t;

/*
 * SOO callback functions.
 * The following definitions are used as argument in domcalls or in the
 * agency_ctl() function as a callback to be propagated to a specific ME.
 *
 */

#define CB_PRE_SUSPEND 4
#define CB_PRE_RESUME 5
#define CB_POST_ACTIVATE 6
#define CB_AGENCY_CTL 9

typedef struct soo_domcall_arg {
	/* Stores the agency ctl function.
	 * Possibly, the agency_ctl function can be associated to a callback operation asked by a ME
	 */
	unsigned int cmd;
	unsigned int slotID; /* Origin of the domcall */

	union {
		pre_suspend_args_t pre_suspend_args;
		pre_resume_args_t pre_resume_args;

		post_activate_args_t post_activate_args;
		ME_state_t set_me_state_args;
	} u;

} soo_domcall_arg_t;

extern struct semaphore usr_feedback;
extern struct semaphore injection_sem;

/* Callbacks initiated by agency ping */
void cb_pre_resume(soo_domcall_arg_t *args);
void cb_pre_suspend(soo_domcall_arg_t *args);

void cb_cooperate(soo_domcall_arg_t *args);
void cb_post_activate(soo_domcall_arg_t *args);

void cb_force_terminate(void);

void callbacks_init(void);

void set_dc_event(domid_t domid, dc_event_t dc_event);

void do_soo_activity(void *arg);

void soo_guest_activity_init(void);

void dc_stable(int dc_event);
void tell_dc_stable(int dc_event);

void do_sync_dom(int slotID, dc_event_t);
void do_async_dom(int slotID, dc_event_t);

void perform_task(dc_event_t dc_event);

void shutdown_ME(unsigned int ME_slotID);

void cache_flush_all(void);

#endif /* __ASSEMBLY__ */

#endif /* UAPI_SOO_H */
