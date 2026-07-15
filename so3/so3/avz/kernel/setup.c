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
#include <memory.h>
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

/*
 * Structural guard: the AVZ footprint, i.e. the kernel image including the
 * heap followed by the frame table ([phys_base .. phys_base + kernel_size]),
 * must never overlap the agency memslot. Growing CONFIG_HEAP_SIZE or the
 * platform RAM size (hence the frame table) can silently push the AVZ
 * footprint into the agency slot and corrupt the guest image.
 */
static void check_avz_agency_overlap(void)
{
	addr_t avz_start = memslot[MEMSLOT_AVZ].base_paddr;
	addr_t avz_end = avz_start + get_kernel_size();
	addr_t agency_start = memslot[MEMSLOT_AGENCY].base_paddr;
	addr_t agency_end = agency_start + memslot[MEMSLOT_AGENCY].size;

	if ((avz_start < agency_end) && (agency_start < avz_end))
		panic("AVZ footprint [0x%lx - 0x%lx] (kernel + heap + frame table) overlaps the agency memslot "
		      "[0x%lx - 0x%lx]; reduce CONFIG_HEAP_SIZE or move the agency load address.\n",
		      avz_start, avz_end, agency_start, agency_end);

	/* The headroom is thinner than it looks (a couple of MB on virt64) and
	 * shrinks with the platform RAM size, since the frame table scales with
	 * it: keep it visible at every boot rather than only when it is too late.
	 */

	lprintk("  AVZ footprint: %ld MB, %ld MB of headroom before the agency slot at 0x%lx\n", get_kernel_size() / SZ_1M,
		(agency_start - avz_end) / SZ_1M, agency_start);
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

	/* The frame table is now placed; make sure AVZ does not spill over the agency slot. */

	check_avz_agency_overlap();

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

	printk("All secondary CPUs are up; unpausing the agency domain...\n");

	domain_unpause_by_systemcontroller(agency);

	set_current_domain(idle_domain[smp_processor_id()]);

	startup_cpu_idle_loop();
}
