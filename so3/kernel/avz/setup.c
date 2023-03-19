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
#include <softirq.h>
#include <console.h>
#include <smp.h>
#include <version.h>
#include <percpu.h>

#include <avz/sched.h>
#include <avz/domain.h>
#include <avz/memslot.h>
#include <avz/keyhandler.h>
#include <avz/event.h>

#include <device/device.h>

#include <asm/processor.h>
#include <asm/io.h>
#include <asm/setup.h>

#define DEBUG

extern void startup_cpu_idle_loop(void);

struct domain *idle_domain[CONFIG_NR_CPUS];

/* Temporary until main.c is unified */
void *app_thread_main(void *args) {
	return NULL;
}

void dump_backtrace_entry(unsigned long where, unsigned long from)
{
	printk("Function entered at [<%08lx>] from [<%08lx>]\n", where, from);
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

#ifndef CONFIG_ARM64VT
	memset(&pseudo_usr_mode, 0, CONFIG_NR_CPUS * sizeof(unsigned int));
#endif

	lprintk("\n\n********** Smart Object Oriented technology - AVZ Hypervisor  **********\n");
	lprintk("Copyright (c) 2014-2023 REDS Institute, HEIG-VD, Yverdon-les-Bains\n");
	lprintk("Version %s\n", SO3_KERNEL_VERSION);

	lprintk("\n\nNow bootstraping the hypervisor kernel ...\n");

	/* Memory manager subsystem initialization */
	memory_init();

	devices_init();

	timer_init();

	local_irq_disable();

	initialize_keytable();

	percpu_init_areas();

	softirq_init();

	/* allocate pages for per-cpu areas */
	for (i = 0; i < CONFIG_NR_CPUS; i++)
		init_percpu_area(i);

	/* Prepare to adapt the serial virtual address at a better location in the I/O space. */
	console_init_post();

	printk("Init domain scheduler...\n");
	domain_scheduler_init();

	printk("Initializing avz timer...\n");

	/* create idle domain */
	init_idle_domain();

	printk("This configuration will spin up at most %d total processors ...\n", CONFIG_NR_CPUS);

#ifdef CONFIG_SOO
	/*
	 * We need to create a sub-domain associated to the realtime CPU so that
	 * hypercalls and upcalls will be processed correctly.
	 */

	domains[DOMID_AGENCY_RT] = domain_create(DOMID_AGENCY_RT, AGENCY_RT_CPU);

	if (domains[DOMID_AGENCY_RT] == NULL)
		panic("Error creating realtime agency subdomain.\n");
#endif /* CONFIG_SOO */

	/* Create initial domain 0. */
	domains[DOMID_AGENCY] = domain_create(DOMID_AGENCY, AGENCY_CPU);
	agency = domains[DOMID_AGENCY];

	if (agency == NULL)
		panic("Error creating primary Agency domain\n");

	if (construct_agency(domains[DOMID_AGENCY]) != 0)
		panic("Could not set up agency guest OS\n");

	/* Check that we do have a agency at this point, as we need it. */
	if (agency == NULL) {
		printk("No agency found, stopping here...\n");
		while (1);
	}

	/* Allow context switch between domains */
	local_irq_enable();

	smp_init();

#ifdef CONFIG_ARCH_ARM32
	/* Enabling VFP module on this CPU */
	vfp_enable();
#endif

	printk("Now, unpausing the agency domain and doing its bootstrap...\n");

	domain_unpause_by_systemcontroller(agency);

	set_current_domain(idle_domain[smp_processor_id()]);

	startup_cpu_idle_loop();

}

