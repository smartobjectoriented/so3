/*
 * Copyright (C) 2014-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <bitops.h>
#include <types.h>
#include <softirq.h>
#include <string.h>

#include <asm/processor.h>

#include <device/irq.h>

static volatile bool softirq_stat[CONFIG_NR_CPUS][NR_SOFTIRQS];

static softirq_handler softirq_handlers[NR_SOFTIRQS];

DEFINE_SPINLOCK(softirq_pending_lock);

/*
 * Perform actions of related pending softirqs if any.
 */
void do_softirq(void)
{
	unsigned int i, cpu;
	unsigned int loopmax;

	loopmax = 0;

	cpu = smp_processor_id();

	while (true) {

		spin_lock(&softirq_pending_lock);

		for (i = 0; i < NR_SOFTIRQS; i++)
			if (softirq_stat[cpu][i])
				break;

		if (i == NR_SOFTIRQS) {
			spin_unlock(&softirq_pending_lock);
			break;
		}

		if (loopmax > 100)   /* Probably something wrong ;-) */
			printk("%s: Warning trying to process softirq on cpu %d for quite a long time (i = %d)...\n", __func__, cpu, i);

		softirq_stat[cpu][i] = false;

		spin_unlock(&softirq_pending_lock);

		(*softirq_handlers[i])();

		/* If we left the interrupt context along a context switch... */
		__in_interrupt = true;

		loopmax++;
	}

	/* schedule() could have been invoked outside a softirq context, therefore we disable the interrupt context here. */
	__in_interrupt = false;
}

void register_softirq(int nr, softirq_handler handler)
{
    ASSERT(nr < NR_SOFTIRQS);

    softirq_handlers[nr] = handler;
}

#ifdef CONFIG_SMP /* CONFIG_SMP */

/*
 * The softirq_pending mask (irqstat) must be coherent between the agency CPUs and MEs CPU
 * since they do not run in the same OS environment, the hardware cache coherency is not guaranteed.
 */
void cpu_raise_softirq(unsigned int cpu, unsigned int nr)
{
	softirq_stat[cpu][nr] = true;

	smp_trigger_event(cpu);
}

#endif /* CONFIG_SMP */

void raise_softirq(unsigned int nr)
{
	softirq_stat[smp_processor_id()][nr] = true;
}


void softirq_init(void)
{
	memset((void *) softirq_stat, 0, sizeof(softirq_stat));
}
