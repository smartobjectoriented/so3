/*
 * Copyright (C) 2014-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <common.h>
#include <log.h>
#include <softirq.h>
#include <console.h>
#include <smp.h>
#include <version.h>
#include <percpu.h>
#include <initcall.h>

#include <avz/sched.h>
#include <avz/domain.h>
#include <avz/memslot.h>
#include <avz/keyhandler.h>
#include <avz/evtchn.h>

#include <device/device.h>
#include <device/arch/gic.h>

#include <asm/processor.h>
#include <asm/io.h>
#include <asm/setup.h>

#define DEBUG

extern void startup_cpu_idle_loop(void);

struct domain *idle_domain[CONFIG_NR_CPUS];

/* Temporary until main.c is unified */
void *app_thread_main(void *args)
{
	return NULL;
}

void dump_backtrace_entry(unsigned long where, unsigned long from)
{
	LOG_DEBUG("Function entered at [<%08lx>] from [<%08lx>]", where, from);
}

void init_idle_domain(void)
{
	int cpu = smp_processor_id();

	/* Domain creation requires that scheduler structures are initialised. */
	idle_domain[cpu] = domain_create(DOMID_IDLE, cpu);

	if (idle_domain[cpu] == NULL)
		BUG();

	set_current_domain(idle_domain[cpu]);
}

void avz_start(void)
{
	int i;

	/* Parse the domain DT and load the loadables images from ITB (AVZ DT, Linux agency). */
	loadAgency();

	lprintk("\n\n********** Smart Object Oriented technology - AVZ Hypervisor  **********\n");
	lprintk("Copyright (c) 2014-2026 REDS Institute, HEIG-VD, Yverdon-les-Bains\n");
	lprintk("Version %s\n", SO3_KERNEL_VERSION);

	LOG_INFO("\n\nNow bootstraping the hypervisor kernel ...\n");

	/* Memory manager subsystem initialization */
	memory_init();

	percpu_init_areas();

	/* allocate pages for per-cpu areas */
	for (i = 0; i < CONFIG_NR_CPUS; i++)
		init_percpu_area(i);

	devices_init();

	timer_init();

	local_irq_disable();

	initialize_keytable();

	softirq_init();

	/* Prepare to adapt the serial virtual address at a better location in the I/O space. */
	console_init_post();

	LOG_DEBUG("Init domain scheduler...");
	domain_scheduler_init();

	LOG_DEBUG("Initializing avz timer...");

	/* create idle domain */
	init_idle_domain();

	LOG_DEBUG("This configuration will spin up at most %d total processors ...", CONFIG_NR_CPUS);

	/* Create initial domain 0 called agency */
	domains[DOMID_AGENCY] = domain_create(DOMID_AGENCY, AGENCY_CPU);
	agency = domains[DOMID_AGENCY];

	if (agency == NULL)
		panic("Error creating primary Agency domain");

	if (construct_agency(domains[DOMID_AGENCY]) != 0)
		panic("Could not set up agency guest OS");

	/* Check that we do have a agency at this point, as we need it. */
	if (agency == NULL)
		panic("No agency found, stopping here...");

	/* Allow context switch between domains */
	local_irq_enable();

	smp_init();

	/* TEMP (rpi4 bring-up diagnostics): the EL2 periodic tick never fired
	 * on the Pi. Dump the timer + GIC state now and again ~100 ms later
	 * (CNTPCT busy-wait): if GICD_ISPENDR0 bit 26 is set in the second
	 * dump, CNTHP fires but is not delivered (routing/CPU-interface
	 * problem); if it stays clear, the timer itself is not programmed or
	 * not counting. Run BEFORE the agency unpause and with IRQs masked:
	 * a first attempt after the unpause got preempted mid-print by the
	 * reschedule into the guest, which (with no tick) never gave the CPU
	 * back. To be removed once the rpi4_64 agency boots. */
	local_irq_disable();
	{
		int pass;
		u64 t0, tfrq;

		for (pass = 0; pass < 2; pass++) {
			printk("GIC/TIMER dump #%d: CNTFRQ=%lu CNTHP_CTL=0x%lx CNTHP_TVAL=0x%lx CNTPCT=0x%lx\n", pass,
			       read_sysreg(cntfrq_el0), read_sysreg(cnthp_ctl_el2), read_sysreg(cnthp_tval_el2),
			       read_sysreg(cntpct_el0));
			printk("  GICD: CTLR=0x%08x ISENABLER0=0x%08x ISPENDR0=0x%08x IGROUPR0=0x%08x\n",
			       ioread32(&gic->gicd->ctlr), ioread32(&gic->gicd->isenabler[0]),
			       ioread32(&gic->gicd->ispendr[0]), ioread32(&gic->gicd->igroupr[0]));
			printk("  GICD: ISACTIVER0=0x%08x IPRIORITYR6=0x%08x RPR=0x%08x HCR_EL2=0x%lx\n",
			       ioread32(&gic->gicd->isactiver[0]), ioread32(&gic->gicd->ipriorityr[6]),
			       ioread32(&gic->gicc->rpr), read_sysreg(hcr_el2));
			printk("  GICC: CTLR=0x%08x PMR=0x%08x HPPIR=0x%08x DAIF=0x%lx\n", ioread32(&gic->gicc->ctlr),
			       ioread32(&gic->gicc->pmr), ioread32(&gic->gicc->hppir), read_sysreg(daif));

			if (pass == 0) {
				tfrq = read_sysreg(cntfrq_el0);
				t0 = read_sysreg(cntpct_el0);

				/* ~100 ms busy wait */
				while (tfrq && ((read_sysreg(cntpct_el0) - t0) < (tfrq / 10)))
					;
			}
		}
	}
	local_irq_enable();

	printk("All secondary CPUs are up; unpausing the agency domain...\n");

	domain_unpause_by_systemcontroller(agency);

	set_current_domain(idle_domain[smp_processor_id()]);

	startup_cpu_idle_loop();
}
