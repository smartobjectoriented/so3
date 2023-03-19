/*
 * Copyright (C) 2014-2022 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <spinlock.h>
#include <softirq.h>
#include <smp.h>
#include <sizes.h>
#include <percpu.h>
#include <memory.h>

#ifdef CONFIG_AVZ
#include <avz/event.h>
#include <avz/domain.h>
#endif

#include <device/irq.h>
#include <device/timer.h>

#include <device/arch/arm_timer.h>
#include <device/arch/gic.h>

#include <asm/setup.h>
#include <asm/cacheflush.h>
#include <asm/vfp.h>
#include <asm/processor.h>
#include <asm/mmu.h>

#ifdef CONFIG_CPU_SPIN_TABLE
#include <mach/io.h>
#endif

static volatile int booted[CONFIG_NR_CPUS] = {0};

DEFINE_PER_CPU(spinlock_t, softint_lock);

#ifdef CONFIG_AVZ
extern void startup_cpu_idle_loop(void);
extern void init_idle_domain(void);
#endif

/*
 * control for which core is the next to come out of the secondary
 * boot "holding pen"
 */
volatile int pen_release = -1;

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
void write_pen_release(int val)
{
	pen_release = val;
	smp_mb();

	flush_dcache_all();
}

int read_pen_release(void) {
	smp_mb();

	flush_dcache_all();

	return pen_release;
}

void smp_trigger_event(int target_cpu)
{
	int cpu = smp_processor_id();
	long cpu_mask = 1 << target_cpu;

	spin_lock(&per_cpu(softint_lock, cpu));

	/* We keep forcing a send of IPI since the other CPU could be in WFI in an idle loop */

	smp_cross_call(cpu_mask, IPI_EVENT_CHECK);

	spin_unlock(&per_cpu(softint_lock, cpu));
}

/***************************/

struct secondary_data secondary_data;
extern  void periodic_timer_start(void);
/*
 * This is the secondary CPU boot entry.  We're using this CPUs
 * idle thread stack, but a set of temporary page tables.
 */
void secondary_start_kernel(void)
{
	unsigned int cpu = smp_processor_id();

#ifdef CONFIG_ARCH_ARM32
	cpu_init();
#endif

	gicc_init();

	printk("CPU%u: Booted secondary processor\n", cpu);

#if defined(CONFIG_AVZ) && defined(CONFIG_ARM64VT)

#ifdef CONFIG_SOO
	if (cpu == AGENCY_RT_CPU) {
		__mmu_switch_kernel((void *) current_domain->avz_shared->pagetable_paddr, true);
#else
		__mmu_switch_kernel((void *) domains[DOMID_AGENCY]->avz_shared->pagetable_paddr, true);
#endif /* CONFIG_SOO */

		booted[cpu] = 1;

#ifdef CONFIG_CPU_SPIN_TABLE
		switch (cpu) {
		case 1:
			pre_ret_to_el1_with_spin(CPU1_RELEASE_ADDR);
			break;
		case 2:
			pre_ret_to_el1_with_spin(CPU2_RELEASE_ADDR);
			break;
		case 3:
			pre_ret_to_el1_with_spin(CPU3_RELEASE_ADDR);
			break;
		default:
			printk("%s: trying to start CPU %d that is not supported.\n", __func__, cpu);
		}

#else
		pre_ret_to_el1();
#endif

#ifdef CONFIG_SOO
	}
#endif /* CONFIG_SOO */

#endif /* CONFIG_AVZ+ARM64VT */

	secondary_timer_init();

	smp_mb();

	booted[cpu] = 1;

	printk("CPU%d booted...\n", cpu);

#ifdef CONFIG_AVZ
	init_idle_domain();
#endif

	/* Enabling VFP module on this CPU */
#ifdef CONFIG_ARCH_ARM32
	vfp_enable();
#endif

	printk("%s: entering idle loop...\n", __func__);

	periodic_timer_start();

#ifdef CONFIG_AVZ
	/* Prepare an idle domain and starts the idle loop */
	startup_cpu_idle_loop();
#endif

	/* Never returned at this point ... */

}

void cpu_up(unsigned int cpu)
{
	/*
	 * We need to tell the secondary core where to find
	 * its stack and the page tables.
	 */

	switch (cpu) {
	case AGENCY_RT_CPU:
		secondary_data.stack = (void *) __cpu1_stack;
		break;

	default:
		secondary_data.stack = (void *) __cpu3_stack;
	}

	secondary_data.pgdir = __pa(__sys_root_pgtable);

	flush_dcache_all();

	/*
	 * Now bring the CPU into our world.
	 */
	smp_boot_secondary(cpu);

#ifdef CONFIG_ARCH_ARM32
	/* Some platforms may require to be kicked off this way. */
	smp_trigger_event(cpu);
#endif

	/*
	 * CPU was successfully started, wait for it
	 * to come online or time out.
	 */

	smp_mb();

	while (!booted[cpu]) ;

	printk("%s finished waiting...\n", __func__);

	secondary_data.stack = NULL;
	secondary_data.pgdir = 0;
}


/******************************************************************************/
/* From linux kernel/smp.c */

/* Called by boot processor to activate the rest. */
void smp_init(void)
{
#if defined(CONFIG_AVZ) && defined(CONFIG_ARM64VT)
	int i;
#endif

#if defined(CONFIG_AVZ)

	/* We re-create a small identity mapping to allow the hypervisor
	 * to bootstrap correctly on other CPUs.
	 * The size must be enough to reach the stack.
	 */

	create_mapping(NULL, CONFIG_RAM_BASE, CONFIG_RAM_BASE, SZ_32M, false);

#ifdef CONFIG_SOO

	printk("CPU #%d is the second CPU reserved for Agency realtime activity.\n", AGENCY_RT_CPU);

		/* Since the RT domain is never scheduled, we set the current domain bound to
		 * CPU #1 to this unique domain.
		 */

	per_cpu(current_domain, AGENCY_RT_CPU) = domains[DOMID_AGENCY_RT];

#ifdef CONFIG_ARM64VT
	printk("Preparing Agency RT CPU to be ready to start...\n");

	cpu_up(AGENCY_RT_CPU);
#endif

	printk("Starting ME CPU...\n");

	cpu_up(ME_CPU);

	printk("Brought secondary CPUs for AVZ (at the moment CPU #3, CPU #2 will be for later...)\n");

#else /* CONFIG_SOO */

#ifdef CONFIG_ARM64VT
	/* With VT support, we prepare the CPU to be started through a HVC call
	 * from the domain.
	 */
	for (i = 1; i < CONFIG_NR_CPUS; i++)
		cpu_up(i);
#endif

#endif /* !CONFIG_SOO */

#endif /* CONFIG_AVZ */

}



