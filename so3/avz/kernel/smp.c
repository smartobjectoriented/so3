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

#include <spinlock.h>
#include <softirq.h>
#include <smp.h>
#include <sizes.h>
#include <percpu.h>
#include <memory.h>

#include <avz/evtchn.h>
#include <avz/domain.h>

#include <avz/uapi/avz.h>

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

static volatile int booted[CONFIG_NR_CPUS] = { 0 };

DEFINE_PER_CPU(spinlock_t, softint_lock);

extern void startup_cpu_idle_loop(void);
extern void init_idle_domain(void);

/*
 * Control for which core is the next to come out of the secondary
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

int read_pen_release(void)
{
	smp_mb();

	flush_dcache_all();

	return pen_release;
}

void smp_trigger_event(int target_cpu)
{
	int cpu = smp_processor_id();
	long cpu_mask = 1 << target_cpu;

	spin_lock(&per_cpu(softint_lock, cpu));
	smp_cross_call(cpu_mask, IPI_EVENT_CHECK);
	spin_unlock(&per_cpu(softint_lock, cpu));
}

/***************************/

struct secondary_data secondary_data;
extern void periodic_timer_start(void);
/*
 * This is the secondary CPU boot entry.  We're using this CPUs
 * idle thread stack, but a set of temporary page tables.
 */
void secondary_start_kernel(void)
{
	unsigned int cpu = smp_processor_id();

	gicc_init();

#ifndef CONFIG_SOO
	__mmu_switch_kernel((void *) domains[DOMID_AGENCY]->pagetable_paddr, true);
#endif /* !CONFIG_SOO */

	booted[cpu] = 1;

#ifdef CONFIG_CPU_SPIN_TABLE
	switch (cpu) {
	case 1:
		pre_ret_to_el1_spin(CPU1_RELEASE_ADDR);
		break;
	case 2:
		pre_ret_to_el1_spin(CPU2_RELEASE_ADDR);
		break;
	case 3:
		pre_ret_to_el1_spin(CPU3_RELEASE_ADDR);
		break;
	default:
		printk("%s: trying to start CPU %d that is not supported.\n", __func__, cpu);
	}
#endif

#ifdef CONFIG_CPU_PSCI
#ifdef CONFIG_SOO
	if (cpu != S3C_CPU)
#endif /* CONFIG_SOO */
	{
		pre_ret_to_el1();
	}
#endif /* CONFIG_CPU_PCSI */

	secondary_timer_init();

	smp_mb();

	booted[cpu] = 1;

	printk("CPU%d booted...\n", cpu);

	init_idle_domain();

	printk("%s: entering idle loop...\n", __func__);

	/* On CPU running containers, the timer IRQ is catched by
	 * the hypervisor and forwarded to guests as virtual IRQ
	 */
	periodic_timer_start();

	/* Prepare an idle domain and starts the idle loop */
	startup_cpu_idle_loop();

	/* Never returned at this point ... */
}

void cpu_up(unsigned int cpu)
{
	unsigned int cpu_stack_size;

	/* We need to reach the top of the stack, so lets jump over
         * the full kernel stack allocated to each CPU in so3.lds.
         */
	cpu_stack_size = (CONFIG_SYS_STACK_SIZE_KB + CONFIG_MAX_THREADS * CONFIG_THREAD_STACK_SIZE_KB) * SZ_1K;

	secondary_data.stack = ((void *) __stack_bottom) + (cpu + 1) * cpu_stack_size;

	secondary_data.pgdir = __pa(__sys_root_pgtable);

	flush_dcache_all();

	/*
	 * Now bring the CPU into our world.
	 */
	smp_boot_secondary(cpu);

	/*
	 * CPU was successfully started, wait for it
	 * to come online or time out.
	 */

	smp_mb();

	while (!booted[cpu])
		;

	secondary_data.stack = NULL;
	secondary_data.pgdir = 0;
}

/******************************************************************************/
/* From linux kernel/smp.c */

/* Called by boot processor to activate the rest. */
void smp_init(void)
{
	int i;

	for (i = 0; i < CONFIG_NR_CPUS; i++)
		spin_lock_init(&per_cpu(softint_lock, i));

	/* We re-create a small identity mapping to allow the hypervisor
	 * to bootstrap correctly on other CPUs.
	 * The size must be enough to reach the stack.
	 */

	create_mapping(NULL, mem_info.phys_base, mem_info.phys_base, SZ_128M, false);

#ifdef CONFIG_SOO

	printk("Starting capsule CPU...\n");

	cpu_up(S3C_CPU);

	printk("Brought secondary CPU %d for running SO3 capsules...\n", S3C_CPU);

#else /* CONFIG_SOO */

	for (i = 1; i < CONFIG_NR_CPUS; i++)
		cpu_up(i);

#endif /* !CONFIG_SOO */
}
