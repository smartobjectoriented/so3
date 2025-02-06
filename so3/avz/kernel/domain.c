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

#if 0
#define DEBUG
#endif

#include <common.h>
#include <stdarg.h>
#include <percpu.h>
#include <serial.h>
#include <console.h>
#include <errno.h>
#include <softirq.h>
#include <memory.h>
#include <heap.h>

#include <asm/processor.h>
#include <asm/vfp.h>

#include <avz/domain.h>
#include <avz/sched.h>
#include <avz/sched-if.h>
#include <avz/debug.h>
#include <avz/gnttab.h>

#include <avz/uapi/avz.h>

static DEFINE_SPINLOCK(domctl_lock);

/*
 * We don't care of the IDLE domain here...
 * In the domain table, the index 0 and 1 are dedicated to the non-RT and RT agency domains.
 * The indexes 1..MAX_DOMAINS are for the MEs. ME_slotID should correspond to domain ID.
 */
struct domain *domains[MAX_DOMAINS];

struct domain *agency;

/*
 * Creation of new domain context associated to the agency or a Mobile Entity.
 *
 * @domid is the domain number
 * @cpu_id the CPU on which this domain is allowed to run
 */
struct domain *domain_create(domid_t domid, int cpu_id)
{
	struct domain *d;

	d = memalign(sizeof(struct domain), 8);
	BUG_ON(!d);

	memset(d, 0, sizeof(struct domain));

	/*
	 * Allocate the shared memory page which is shared between the hypervisor
	 * and the domain.
	 */
	d->avz_shared = memalign(PAGE_SIZE, PAGE_SIZE);
	BUG_ON(!d->avz_shared);

	memset(d->avz_shared, 0, PAGE_SIZE);

	/*
	 * Grant table and grant pages
	 * Each domain has pre-defined number of pages used to share data
	 * between domains, especially between the Linux domain and the SO3 containers.
	 * 
	 */

	gnttab_init(d);

	d->avz_shared->domID = domid;

	if (!is_idle_domain(d)) {
		d->is_paused_by_controller = 1;
		atomic_inc(&d->pause_count);

		evtchn_init(d);
	}

	arch_domain_create(d, cpu_id);

	d->processor = cpu_id;

	spin_lock_init(&d->virq_lock);

	if (is_idle_domain(d)) {
		d->runstate = RUNSTATE_running;
	} else {
		d->runstate = RUNSTATE_offline;
		set_bit(_VPF_down, &d->pause_flags);
	}

	/* Now, we assign a scheduling policy for this domain */

	if (is_idle_domain(d) && (cpu_id == AGENCY_CPU))
		d->sched = &sched_agency;
	else {
		if (cpu_id == ME_CPU) {
			d->sched = &sched_flip;
			d->need_periodic_timer = true;

		} else if (cpu_id == AGENCY_CPU) {
			d->sched = &sched_agency;
			d->need_periodic_timer = true;

		} else if (cpu_id == AGENCY_RT_CPU)

			d->sched = &sched_agency;
	}

	if (sched_init_domain(d, cpu_id) != 0)
		BUG();

	return d;
}

/* Complete domain destroy */
static void complete_domain_destroy(struct domain *d)
{
	sched_destroy_domain(d);

	/* Remove the root page table */
	reset_root_pgtable((void *)d->pagetable_vaddr, true);

	/* Restore allocated memory for this domain */

	free((void *)d->avz_shared);
	free((void *)d->domain_stack);

	free(d);
}

/* Release resources belonging to a domain */
void domain_destroy(struct domain *d)
{
	BUG_ON(!d->is_dying);

	complete_domain_destroy(d);
}

void vcpu_pause(struct domain *d)
{
	ASSERT(d != current_domain);
	atomic_inc(&d->pause_count);
	vcpu_sleep_sync(d);
}

void vcpu_pause_nosync(struct domain *d)
{
	atomic_inc(&d->pause_count);
	vcpu_sleep_nosync(d);
}

void vcpu_unpause(struct domain *d)
{
	if (atomic_dec_and_test(&d->pause_count))
		vcpu_wake(d);
}

void domain_pause(struct domain *d)
{
	ASSERT(d != current_domain);

	atomic_inc(&d->pause_count);

	vcpu_sleep_sync(d);
}

void domain_unpause(struct domain *d)
{
	if (atomic_dec_and_test(&d->pause_count))
		vcpu_wake(d);
}

void domain_pause_by_systemcontroller(struct domain *d)
{
	/* We must ensure that the domain is not already paused */
	BUG_ON(d->is_paused_by_controller);

	if (!test_and_set_bool(d->is_paused_by_controller))
		domain_pause(d);
}

void domain_unpause_by_systemcontroller(struct domain *d)
{
	if (test_and_clear_bool(d->is_paused_by_controller))
		domain_unpause(d);
}

/**
 * @brief Perform a context switch between domains. It is equivalent
 * 	  to say that we switch the VM.
 * 
 * @param prev 
 * @param next 
 */
void context_switch(struct domain *prev, struct domain *next)
{
	local_irq_disable();

	if (!is_idle_domain(current_domain)) {
		local_irq_disable(); /* Again, if the guest re-enables the IRQ */
	}

	vcpu_save_context(prev);

	switch_mm_domain(next);

	vcpu_restore_context(next);

	/* Clear running flag /after/ writing context to memory. */
	smp_mb();

	prev->is_running = 0;

	/* Check for migration request /after/ clearing running flag. */
	smp_mb();

	spin_unlock(&prev->sched->sched_data.schedule_lock);

	__switch_domain_to(prev, next);
}

static void continue_cpu_idle_loop(void)
{
	while (1) {
		local_irq_disable();

		raise_softirq(SCHEDULE_SOFTIRQ);
		do_softirq();

		ASSERT(local_irq_is_disabled());

		local_irq_enable();

		cpu_do_idle();
	}
}

void startup_cpu_idle_loop(void)
{
	ASSERT(is_idle_domain(current_domain));

	raise_softirq(SCHEDULE_SOFTIRQ);

	continue_cpu_idle_loop();
}

void machine_halt(void)
{
	printk("machine_halt called: spinning....\n");

	while (1)
		;
}

void machine_restart(unsigned int delay_millisecs)
{
	printk("machine_restart called: spinning....\n");

	while (1)
		;
}

void do_domctl(domctl_t *args)
{
	struct domain *d;

	spin_lock(&domctl_lock);

	d = domains[args->domain];

	switch (args->cmd) {
	case DOMCTL_pauseME:

		domain_pause_by_systemcontroller(d);
		break;

	case DOMCTL_unpauseME:

		DBG("%s: unpausing ME\n", __func__);

		domain_unpause_by_systemcontroller(d);
		break;

	case DOMCTL_get_AVZ_shared:
		if (current_domain->avz_shared->domID == DOMID_AGENCY)
			args->avz_shared_paddr =
				memslot[MEMSLOT_AGENCY].ipa_addr +
				memslot[MEMSLOT_AGENCY].size;
		else
			args->avz_shared_paddr =
				memslot[current_domain->avz_shared->domID]
					.ipa_addr +
				memslot[current_domain->avz_shared->domID].size;
		break;
	}

	spin_unlock(&domctl_lock);
}
