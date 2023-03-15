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

#ifndef AVZ_H
#define AVZ_H

#include <avz/uapi/soo.h>

#include <asm/atomic.h>

/*
 * AVZ HYPERCALLS
 */

#define __HYPERVISOR_event_channel_op      0
#define __HYPERVISOR_console_io            1
#define __HYPERVISOR_physdev_op            2
#define __HYPERVISOR_sched_op              3
#define __HYPERVISOR_domctl                4
#define __HYPERVISOR_soo_hypercall         5

/*
 * VIRTUAL INTERRUPTS
 *
 * Virtual interrupts that a guest OS may receive from the hypervisor.
 *
 */
#define	NR_VIRQS	8

#define VIRQ_TIMER      0  /* System timer tick virtualized interrupt */
#define VIRQ_TIMER_RT   1  /* Timer tick issued from the oneshot timer (for RT agency and MEs */

/**************************************************/

/*
 * Commands to HYPERVISOR_console_io().
 */
#define CONSOLEIO_write_string  0
#define CONSOLEIO_process_char  1

/* Idle domain. */
#define DOMID_IDLE (0x7FFFU)

/* DOMID_SELF is used in certain contexts to refer to oneself. */
#define DOMID_SELF (0x7FF0U)

/* Agency */
#define DOMID_AGENCY	0

/* Realtime agency subdomain */
#define DOMID_AGENCY_RT	1

extern int hypercall_trampoline(int hcall, long a0, long a2, long a3, long a4);

/*
 * 128 event channels per domain
 */
#define NR_EVTCHN 128

/*
 * Shared info page, shared between AVZ and the domain.
 */
struct avz_shared {

	domid_t domID;

	/* Domain related information */
	unsigned long nr_pages;     /* Total pages allocated to this domain.  */

	/* Hypercall vector addr for direct branching without syscall */
	addr_t hypercall_vaddr;

	/* Interrupt routine in the domain */
	addr_t vectors_vaddr;

	/* Domcall routine in the domain */
	addr_t domcall_vaddr;

	/* Trap routine in the domain */
	addr_t traps_vaddr;

	addr_t fdt_paddr;

	/* Low-level print function mainly for debugging purpose */
	void (*printch)(char c);

	/* VBstore pfn */
	unsigned long vbstore_pfn;

	unsigned long dom_phys_offset;

	/* Physical and virtual address of the page table used when the domain is bootstraping */
	addr_t pagetable_paddr;
	addr_t pagetable_vaddr; /* Required when bootstrapping the domain */

	/* Address of the logbool ht_set function which can be used in the domain. */
	unsigned long logbool_ht_set_addr;

	/* We inform the domain about the hypervisor memory region so that the
	 * domain can re-map correctly.
	 */
	addr_t hypervisor_vaddr;

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

	/* Agency or ME descriptor */
	dom_desc_t dom_desc;

	/* Keep the physical address so that the guest can map within in its address space. */
	addr_t subdomain_shared_paddr;

	struct avz_shared *subdomain_shared;

	/* Reference to the logbool hashtable (one per each domain) */
	void *logbool_ht;
};

typedef struct avz_shared avz_shared_t;

extern avz_shared_t *avz_shared;

/*
 * DOMCALLs
 */
typedef void (*domcall_t)(int cmd, void *arg);

#define DOMCALL_sync_vbstore               	1
#define DOMCALL_post_migration_sync_ctrl    	2
#define DOMCALL_sync_domain_interactions    	3
#define DOMCALL_presetup_adjust_variables   	4
#define DOMCALL_postsetup_adjust_variables  	5
#define DOMCALL_fix_other_page_tables		6
#define DOMCALL_sync_directcomm			7
#define DOMCALL_soo				8

struct DOMCALL_presetup_adjust_variables_args {
	avz_shared_t *avz_shared; /* IN */
};

struct DOMCALL_postsetup_adjust_variables_args {
	long pfn_offset;
};

struct DOMCALL_fix_page_tables_args {
	long pfn_offset; /* Offset with which to fix the page table entries */
	unsigned int min_pfn, nr_pages;   /* min_pfn is the physical start address of the target RAM, nr_pages the number of pages */
};

struct DOMCALL_directcomm_args {
	unsigned int directcomm_evtchn;
};

struct DOMCALL_sync_vbstore_args {
	unsigned int vbstore_pfn;  /* OUT */
	unsigned int vbstore_revtchn; /* Agency side */
};

struct DOMCALL_sync_domain_interactions_args {
	unsigned int vbstore_pfn; 	/* IN */
	unsigned int vbstore_levtchn;
};

void postmig_adjust_timer(void);

#ifndef CONFIG_AVZ
#define ME_domID() (avz_shared->domID)
#endif

#endif /* AVZ_H */

