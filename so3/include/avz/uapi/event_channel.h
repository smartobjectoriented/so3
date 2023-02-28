/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@soo.tech>
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

#ifndef __EVENT_CHANNEL_H__
#define __EVENT_CHANNEL_H__

#include <avz/uapi/avz.h>

#define ECS_FREE         0 /* Channel is available for use.                  */
#define ECS_RESERVED     1 /* Channel is reserved.                           */
#define ECS_UNBOUND      2 /* Channel is waiting to bind to a remote domain. */
#define ECS_INTERDOMAIN  3 /* Channel is bound to another domain.            */
#define ECS_VIRQ         4 /* Channel is bound to a virtual IRQ line.        */

#define EVTCHNSTAT_closed       0  /* Channel is not in use.                 */
#define EVTCHNSTAT_unbound      1  /* Channel is waiting interdom connection.*/
#define EVTCHNSTAT_interdomain  2  /* Channel is connected to remote domain. */
#define EVTCHNSTAT_virq         3  /* Channel is bound to a virtual IRQ line */
/*
 * EVTCHNOP_alloc_unbound: Allocate a evtchn in domain <dom> and mark as
 * accepting interdomain bindings from domain <remote_dom>. A fresh evtchn
 * is allocated in <dom> and returned as <evtchn>.
 * NOTES:
 *  1. If the caller is unprivileged then <dom> must be DOMID_SELF.
 *  2. <rdom> may be DOMID_SELF, allowing loopback connections.
 */
#define EVTCHNOP_alloc_unbound    6
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
#define EVTCHNOP_bind_interdomain     	      0
#define EVTCHNOP_bind_existing_interdomain    7
#define EVTCHNOP_unbind_domain 		      12

struct evtchn_bind_interdomain {
    /* IN parameters. */
    domid_t remote_dom;
    uint32_t remote_evtchn;
    uint32_t use;
    /* OUT parameters. */
    uint32_t local_evtchn;
};
typedef struct evtchn_bind_interdomain evtchn_bind_interdomain_t;

#define EVTCHNOP_bind_virq        1
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
#define EVTCHNOP_close            3
struct evtchn_close {
    /* IN parameters. */
    uint32_t evtchn;
};
typedef struct evtchn_close evtchn_close_t;

/*
 * EVTCHNOP_send: Send an event to the remote end of the channel whose local
 * endpoint is <evtchn>.
 */
#define EVTCHNOP_send             4
struct evtchn_send {
    /* IN parameters. */
    uint32_t evtchn;
};
typedef struct evtchn_send evtchn_send_t;

/*
 * EVTCHNOP_status: Get the current status of the communication channel which
 * has an endpoint at <dom, evtchn>.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may obtain the status of an event
 *     channel for which <dom> is not DOMID_SELF.
 */
#define EVTCHNOP_status           5
struct evtchn_status {
    /* IN parameters */
    domid_t  dom;
    uint32_t evtchn;
    /* OUT parameters */

    uint32_t status;

    union {
        struct {
            domid_t dom;
        } unbound; /* EVTCHNSTAT_unbound */
        struct {
            domid_t dom;
            uint32_t evtchn;
        } interdomain; /* EVTCHNSTAT_interdomain */

        uint32_t virq;      /* EVTCHNSTAT_virq        */
    } u;
};
typedef struct evtchn_status evtchn_status_t;

/*
 * EVTCHNOP_unmask: Unmask the specified local event-channel evtchn and deliver
 * a notification to the appropriate VCPU if an event is pending.
 */
#define EVTCHNOP_unmask           9
struct evtchn_unmask {
    /* IN parameters. */
    uint32_t evtchn;
};
typedef struct evtchn_unmask evtchn_unmask_t;

/*
 * EVTCHNOP_reset: Close all event channels associated with specified domain.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may specify other than DOMID_SELF.
 */
#define EVTCHNOP_reset           10
struct evtchn_reset {
    /* IN parameters. */
    domid_t dom;
};
typedef struct evtchn_reset evtchn_reset_t;

struct evtchn_op {
    uint32_t cmd; /* EVTCHNOP_* */
    union {
        struct evtchn_alloc_unbound    alloc_unbound;
        struct evtchn_bind_interdomain bind_interdomain;
        struct evtchn_bind_virq        bind_virq;
        struct evtchn_close            close;
        struct evtchn_send             send;
        struct evtchn_status           status;
        struct evtchn_unmask           unmask;
    } u;
};
typedef struct evtchn_op evtchn_op_t;

#endif /* __EVENT_CHANNEL_H__ */

