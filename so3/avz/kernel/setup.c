/*
 * Copyright (C) 2014-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
	lprintk("Copyright (c) 2014-2025 REDS Institute, HEIG-VD, Yverdon-les-Bains\n");
	lprintk("Version %s\n", SO3_KERNEL_VERSION);

	LOG_INFO("\n\nNow bootstraping the hypervisor kernel ...");

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

	LOG_DEBUG("Now, unpausing the agency domain and doing its bootstrap...");

	domain_unpause_by_systemcontroller(agency);

	set_current_domain(idle_domain[smp_processor_id()]);

	startup_cpu_idle_loop();
}
