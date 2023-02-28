
/*
 * Copyright (C) 2014-2016 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2016 Baptiste Delporte <bonel@bonel.net>
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

#include <types.h>
#include <bitops.h>

#include <device/irq.h>

#include <avz/uapi/avz.h>

#include <soo/hypervisor.h>
#include <soo/evtchn.h>
#include <soo/physdev.h>
#include <soo/console.h>
#include <soo/debug.h>
#include <soo/debug/dbgvar.h>

/*
 * This lock protects updates to the following mapping and reference-count
 * arrays. The lock does not need to be acquired to read the mapping tables.
 */
static DEFINE_SPINLOCK(irq_mapping_update_lock);

static int virq_bindcount[NR_VIRQS];	/* Reference counts for bindings to VIRQs. */

typedef struct {
	int evtchn_to_irq[NR_EVTCHN];	/* evtchn -> IRQ */
	u32 irq_to_evtchn[NR_VIRQS];	/* IRQ -> evtchn */
	bool valid[NR_EVTCHN]; /* Indicate if the event channel can be used for notification for example */
	bool evtchn_mask[NR_EVTCHN];
} evtchn_info_t;

static evtchn_info_t evtchn_info;

bool in_upcall_progress;

inline unsigned int evtchn_from_irq(int irq)
{
	return evtchn_info.irq_to_evtchn[irq];
}

static inline bool evtchn_is_masked(unsigned int b) {
	return evtchn_info.evtchn_mask[b];
}

void dump_evtchn_pending(void) {
	int i;

	printk("   Evtchn info in Agency/ME domain %d\n\n", ME_domID());
	for (i = 0; i < NR_EVTCHN; i++)
		printk("e:%d m:%d p:%d  ", i, evtchn_info.evtchn_mask[i], avz_shared->evtchn_pending[i]);

	printk("\n\n");
}

/*
 * evtchn_do_upcall
 *
 * This is the main entry point for processing IRQs and VIRQs for this domain.
 *
 * The function runs with IRQs OFF during the whole execution of the function. As such, this function is executed in top half processing of the IRQ.
 * - All pending event channels are processed one after the other, according to their bit position within the bitmap.
 * - Checking pending IRQs in AVZ (low-level IRQ) is performed at the end of the loop so that we can react immediately if some new IRQs have been generated
 *   and are present in the GIC.
 *
 * -> For VIRQ_TIMER_IRQ, avoid change the bind_virq_to_irqhandler.....
 *
 */
void evtchn_do_upcall(cpu_regs_t *regs)
{
	unsigned int evtchn;
	int l1, irq;

	int loopmax = 0;

	BUG_ON(local_irq_is_enabled());

	/*
	 * This function has to be reentrant to allow the processing of event channel issued
	 * by the avz softirq processing even if IRQs are off (during an hypercall upcall).
	 * In this case, we check if we are already in a previous interrupt processing.
	 */

	if (in_upcall_progress)
		return ;

	/* Check if the (local) IRQs are off. In this case, pending events are not processed at this time,
	 * but will be once the local IRQs will be re-enabled (either by the GIC loop or an active assert
	 * of the IRQ line).
	 */

	if (irqs_disabled_flags(regs))
		return ;

	in_upcall_progress = true;

retry:

	l1 = xchg(&avz_shared->evtchn_upcall_pending, 0);
	BUG_ON(l1 == 0);

	while (true) {
		for (evtchn = 0; evtchn < NR_EVTCHN; evtchn++)
			if ((avz_shared->evtchn_pending[evtchn]) && !evtchn_is_masked(evtchn))
				break;

		/* Found an evtchn? */
		if (evtchn == NR_EVTCHN)
			break;

		BUG_ON(!evtchn_info.valid[evtchn]);

		loopmax++;

		if (loopmax > 500)   /* Probably something wrong ;-) */
			printk("%s: Warning trying to process evtchn: %d IRQ: %d for quite a long time (dom ID: %d) on CPU %d / masked: %d...\n",
				__func__, evtchn, evtchn_info.evtchn_to_irq[evtchn], ME_domID(), smp_processor_id(), evtchn_is_masked(evtchn));

		irq = evtchn_info.evtchn_to_irq[evtchn];
		clear_evtchn(evtchn_from_irq(irq));

		/* Mask the VIRQ event */
		irq_mask(irq);

		irq_process(irq);

		/* Unmask the VIRQ event channel */
		irq_unmask(irq);

		BUG_ON(local_irq_is_enabled());

	};

	if (avz_shared->evtchn_upcall_pending)
		goto retry;

	in_upcall_progress = false;
}

static int find_unbound_irq(void)
{
	int irq;

	for (irq = 0; irq < NR_IRQS; irq++)
		if (virq_bindcount[irq] == 0)
			break;

	if (irq == NR_IRQS) {
		printk("No available IRQ to bind to: increase NR_IRQS!\n");
		BUG();
	}

	return irq;
}

bool in_upcall_process(void) {
	return in_upcall_progress;
}

static int bind_evtchn_to_virq(unsigned int evtchn)
{
	int irq;

	spin_lock(&irq_mapping_update_lock);

	if ((irq = evtchn_info.evtchn_to_irq[evtchn]) == -1) {
		irq = find_unbound_irq();
		evtchn_info.evtchn_to_irq[evtchn] = irq;
		evtchn_info.irq_to_evtchn[irq] = evtchn;
		evtchn_info.valid[evtchn] = true;
	}

	virq_bindcount[irq]++;

	spin_unlock(&irq_mapping_update_lock);

	return irq;
}

void unbind_domain_evtchn(unsigned int domID, unsigned int evtchn)
{
	struct evtchn_bind_interdomain bind_interdomain;

	bind_interdomain.remote_dom = domID;
	bind_interdomain.local_evtchn = evtchn;

	hypercall_trampoline(__HYPERVISOR_event_channel_op, EVTCHNOP_unbind_domain, (long) &bind_interdomain, 0, 0);

	evtchn_info.valid[evtchn] = false;
}

static int bind_interdomain_evtchn_to_irq(unsigned int remote_domain, unsigned int remote_evtchn)
{
	struct evtchn_bind_interdomain bind_interdomain;

	bind_interdomain.remote_dom  = remote_domain;
	bind_interdomain.remote_evtchn = remote_evtchn;

	hypercall_trampoline(__HYPERVISOR_event_channel_op, EVTCHNOP_bind_interdomain, (long) &bind_interdomain, 0, 0);

	return bind_evtchn_to_virq(bind_interdomain.local_evtchn);
}

int bind_existing_interdomain_evtchn(unsigned local_evtchn, unsigned int remote_domain, unsigned int remote_evtchn)
{
	struct evtchn_bind_interdomain bind_interdomain;

	bind_interdomain.local_evtchn = local_evtchn;
	bind_interdomain.remote_dom  = remote_domain;
	bind_interdomain.remote_evtchn = remote_evtchn;

	hypercall_trampoline(__HYPERVISOR_event_channel_op, EVTCHNOP_bind_existing_interdomain, (long) &bind_interdomain, 0, 0);

	return bind_evtchn_to_virq(bind_interdomain.local_evtchn);
}

void bind_virq(unsigned int virq)
{
	evtchn_bind_virq_t bind_virq;
	int evtchn;
	unsigned long flags;

	flags = spin_lock_irqsave(&irq_mapping_update_lock);

	bind_virq.virq = virq;
	hypercall_trampoline(__HYPERVISOR_event_channel_op, EVTCHNOP_bind_virq, (long) &bind_virq, 0, 0);

	evtchn = bind_virq.evtchn;

	evtchn_info.evtchn_to_irq[evtchn] = virq;
	evtchn_info.irq_to_evtchn[virq] = evtchn;
	evtchn_info.valid[evtchn] = true;

	unmask_evtchn(evtchn);

	virq_bindcount[virq]++;

	spin_unlock_irqrestore(&irq_mapping_update_lock, flags);
}

static void unbind_from_irq(unsigned int irq)
{
	evtchn_close_t op;
	int evtchn = evtchn_from_irq(irq);

	spin_lock(&irq_mapping_update_lock);

	if (--virq_bindcount[irq] == 0) {
		op.evtchn = evtchn;
		hypercall_trampoline(__HYPERVISOR_event_channel_op, EVTCHNOP_close, (long) &op, 0, 0);

		evtchn_info.evtchn_to_irq[evtchn] = -1;
		evtchn_info.valid[evtchn] = false;
	}

	spin_unlock(&irq_mapping_update_lock);
}


int bind_evtchn_to_irq_handler(unsigned int evtchn, irq_handler_t handler, irq_handler_t thread_fn, void *data)
{
	unsigned int irq;

	irq = bind_evtchn_to_virq(evtchn);

	irq_bind(irq, handler, thread_fn, data);

	return irq;
}

int bind_interdomain_evtchn_to_irqhandler(unsigned int remote_domain, unsigned int remote_evtchn, irq_handler_t handler, irq_handler_t thread_fn, void *data)
{
	int irq;

	DBG("%s: remote evtchn: %d remote domain: %d\n", __func__, remote_evtchn, remote_domain);
	irq = bind_interdomain_evtchn_to_irq(remote_domain, remote_evtchn);
	if (irq < 0)
		BUG();

	irq_bind(irq, handler, thread_fn, data);

	return irq;
}

void bind_virq_to_irqhandler(unsigned int virq, irq_handler_t handler, irq_handler_t thread_fn, void *data)
{
	bind_virq(virq);

	irq_bind(virq, handler, thread_fn, data);
}


void unbind_from_irqhandler(unsigned int irq)
{
	unbind_from_irq(irq);

	irq_unbind(irq);
}


/*
 * Send a notification event along an event channel.
 * To send a notification, the
 */
void notify_remote_via_virq(int irq)
{
	int evtchn = evtchn_from_irq(irq);

	BUG_ON(!evtchn_info.valid[evtchn]);

	notify_remote_via_evtchn(evtchn);

}

void mask_evtchn(int evtchn)
{
	evtchn_info.evtchn_mask[evtchn] = true;
}

void unmask_evtchn(int evtchn)
{
	evtchn_info.evtchn_mask[evtchn] = false;
}

void virq_init(void)
{
	int i;
	irqdesc_t *irqdesc;

	/*
	 * For each CPU, initialize event channels for all IRQs.
	 * An IRQ will processed by only one CPU, but it may be rebound to another CPU as well.
	 */

	in_upcall_progress = false;

	/* No event-channel -> IRQ mappings. */
	for (i = 0; i < NR_EVTCHN; i++) {
		evtchn_info.evtchn_to_irq[i] = -1;
		evtchn_info.valid[i] = false;
		mask_evtchn(i); /* No event channels are 'live' right now. */
	}

	/* Dynamic IRQ space is currently unbound. Zero the refcnts. */
	for (i = 0; i < NR_VIRQS; i++)
		virq_bindcount[i] = 0;

	/* Configure the irqdesc associated to VIRQs */
	for (i = 0; i < NR_IRQS; i++) {
		irqdesc = irq_to_desc(i);

		irqdesc->action = NULL;
		irqdesc->irq_deferred_fn = NULL;
	}

	/* Now reserve the pre-defined VIRQ used by AVZ */
	virq_bindcount[VIRQ_TIMER]++;
}
