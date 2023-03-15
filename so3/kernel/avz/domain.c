/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2016-2019 Baptiste Delporte <bonel@bonel.net>
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
#include <avz/logbool.h>

#include <avz/uapi/avz.h>

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
 * @partial tells if the domain creation remains partial, without the creation of the vcpu structure which may intervene in a second step
 */
struct domain *domain_create(domid_t domid, int cpu_id)
{
	struct domain *d;

	d = memalign(sizeof(struct domain), 8);
	BUG_ON(!d);

	memset(d, 0, sizeof(struct domain));

	d->avz_shared = memalign(PAGE_SIZE, PAGE_SIZE);
	BUG_ON(!d);

	memset(d->avz_shared, 0, PAGE_SIZE);

	d->avz_shared->domID = domid;

	if (!is_idle_domain(d)) {
		d->is_paused_by_controller = 1;
		atomic_inc(&d->pause_count);

		evtchn_init(d);
	}

	/* Create a logbool hashtable associated to this domain */
	d->avz_shared->logbool_ht = ht_create(LOGBOOL_HT_SIZE);
	BUG_ON(!d->avz_shared->logbool_ht);

	arch_domain_create(d, cpu_id);

	d->processor = cpu_id;

	spin_lock_init(&d->virq_lock);

	if (is_idle_domain(d))
	{
		d->runstate = RUNSTATE_running;
	}
	else
	{
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

	/* Free the logbool hashtable associated to this domain */
	ht_destroy((logbool_hashtable_t *) d->avz_shared->logbool_ht);

	/* Restore allocated memory for this domain */

	free((void *) d->avz_shared);
	free((void *) d->domain_stack);

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

void domain_pause_by_systemcontroller(struct domain *d) {
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

void context_switch(struct domain *prev, struct domain *next)
{
	local_irq_disable();

	if (!is_idle_domain(current_domain)) {

		local_irq_disable();  /* Again, if the guest re-enables the IRQ */

#ifdef CONFIG_ARCH_ARM32
		/* Save the VFP context */
		vfp_save_state(prev);
#endif
	}

	if (!is_idle_domain(next)) {

#ifdef CONFIG_ARCH_ARM32
		/* Restore the VFP context of the next guest */
		vfp_restore_state(next);
#endif
	}

	switch_mm_domain(next);

	/* Clear running flag /after/ writing context to memory. */
	smp_mb();

	prev->is_running = 0;

	/* Check for migration request /after/ clearing running flag. */
	smp_mb();

	spin_unlock(&prev->sched->sched_data.schedule_lock);

	__switch_domain_to(prev, next);

}

/*
 * Initialize the domain stack used by the hypervisor.
 * This is the H-stack and contains a reference to the domain as the bottom (base) of the stack.
 */
void *setup_dom_stack(struct domain *d) {
	void *domain_stack;

	/* The stack must be aligned at STACK_SIZE bytes so that it is
	 * possible to retrieve the cpu_info structure at the bottom
	 * of the stack with a simple operation on the current stack pointer value.
	 */
	domain_stack = memalign(DOMAIN_STACK_SIZE, DOMAIN_STACK_SIZE);
	BUG_ON(!domain_stack);

	d->domain_stack = (unsigned long) domain_stack;

	/* Put the address of the domain descriptor at the base of this stack */
	*((addr_t *) domain_stack) = (addr_t) d;

	/* Reserve the frame which will be restored later */
	domain_stack += DOMAIN_STACK_SIZE - sizeof(cpu_regs_t);

	/* Returns the reference to the H-stack frame of this domain */

	return domain_stack;
}

/*
 * Set up the first thread of a domain.
 */
void new_thread(struct domain *d, addr_t start_pc, addr_t fdt_addr, addr_t start_stack)
{
	cpu_regs_t *domain_frame;

	domain_frame = (cpu_regs_t *) setup_dom_stack(d);

	if (domain_frame == NULL)
	  panic("Could not set up a new domain stack.n");

	arch_setup_domain_frame(d, domain_frame, fdt_addr, start_stack, start_pc);
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

	while (1);
}

void machine_restart(unsigned int delay_millisecs)
{
	printk("machine_restart called: spinning....\n");

	while (1);
}

/*
 * dommain_call
 *    Run a domain routine from hypervisor
 *    @target_dom is the domain which routine is executed
 *    @current_mapped is the domain which page table is currently loaded.
 *    @current_mapped_mode indicates if we consider the swapper pgdir or the normal page table (see switch_mm() for complete description)
 */
void domain_call(struct domain *target_dom, int cmd, void *arg)
{
	struct domain *__current;
	addr_t prev;

	/* IRQs are always disabled during a domcall */
	BUG_ON(local_irq_is_enabled());

	/* Switch the current domain to the target so that preserving the page table during
	 * subsequent memory context switch will not affect the original one.
	 */

	__current = current_domain;

	/* Preserve the current pgtable address on the AGENCY CPU
	 * since we are about to move to the ME memory context.
	 */
	mmu_get_current_pgtable(&prev);

	switch_mm_domain(target_dom);

	BUG_ON(!target_dom->avz_shared->domcall_vaddr);

	/* Perform the domcall execution */
	((domcall_t) target_dom->avz_shared->domcall_vaddr)(cmd, arg);

	set_current_domain(__current);

	/* Switch back to the same page table (and potential attributes) as at the entry */
	mmu_switch_kernel((void *) prev);

}
