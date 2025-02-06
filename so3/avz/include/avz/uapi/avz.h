/*
 * Copyright (C) 2016-2022 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef UAPI_AVZ_H
#define UAPI_AVZ_H

#ifndef __ASSEMBLY__

#ifdef CONFIG_SOO
#include <soo/uapi/soo.h>
#endif

#include <asm/atomic.h>

#endif /* __ASSEMBLY__ */

/*
 * VIRTUAL INTERRUPTS
 *
 * Virtual interrupts that a guest OS may receive from the hypervisor.
 *
 */
#define NR_VIRQS 256

#define VIRQ_TIMER 0 /* System timer tick virtualized interrupt */
#define VIRQ_TIMER_RT \
	1 /* Timer tick issued from the oneshot timer (for RT agency and MEs */

/**************************************************/

/*
 * Commands to HYPERVISOR_console_io().
 */
#define CONSOLEIO_write_string 0
#define CONSOLEIO_process_char 1

/* Idle domain. */
#define DOMID_IDLE (0x7FFFU)

/* DOMID_SELF is used in certain contexts to refer to oneself. */
#define DOMID_SELF (0x7FF0U)

/* Agency */
#define DOMID_AGENCY 0

/* Realtime agency subdomain */
#define DOMID_AGENCY_RT 1

#define DOMID_INVALID (0x7FF4U)

#define AVZ_HYPERCALL_TRAP 0x2605
#define AVZ_HYPERCALL_SIGRETURN 0x2606

#ifndef __ASSEMBLY__

/* Assembly low-level code to raise up hypercall */
extern long __avz_hypercall(int vector, long avz_hyp_args);

/* Generic function to raise up hypercall */
void avz_hypercall(avz_hyp_t *avz_hyp);

/*
 * 128 event channels per domain
 */
#define NR_EVTCHN 128

#ifndef DOMID_T
#define DOMID_T
typedef uint16_t domid_t;
typedef unsigned long addr_t;
#endif

/*
 * Shared info page, shared between AVZ and the domain.
 */
struct avz_shared {
	domid_t domID;

	/* Domain related information */
	unsigned long nr_pages; /* Total pages allocated to this domain.  */

	addr_t fdt_paddr;

	/* Other fields related to domain life */

	unsigned long domain_stack;
	uint8_t evtchn_upcall_pending;

	/*
	 * A domain can create "event channels" on which it can send and receive
	 * asynchronous event notifications.
	 * Each event channel is assigned a bit in evtchn_pending and its modification has to be
	 * kept atomic.
	 */

	volatile bool evtchn_pending[NR_EVTCHN];

	atomic_t dc_event;

	/* This field is used when taking a snapshot of us. It will be
	 * useful to restore later. Some timer deadlines are based on it and
	 * will need to be updated accordingly.
	 */
	u64 current_s_time;

	/* Agency or ME descriptor */
	dom_desc_t dom_desc;

	/* Keep the physical address so that the guest can map within in its address space. */
	addr_t subdomain_shared_paddr;

	struct avz_shared *subdomain_shared;

	/* Used to store a signature for consistency checking, for example after a migration/restoration */
	char signature[4];
};

typedef struct avz_shared avz_shared_t;

extern volatile avz_shared_t *__avz_shared;

void do_avz_hypercall(avz_hyp_t *args);
void __sigreturn(void);

#endif /* __ASSEMBLY__ */

#endif /* UAPI_AVZ_H */
