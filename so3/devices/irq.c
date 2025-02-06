/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <thread.h>
#include <heap.h>
#include <string.h>
#include <thread.h>

#include <device/irq.h>

#define DEBUG

static irqdesc_t irqdesc[NR_IRQS];
volatile bool __in_interrupt = false;

/* By default IRQ chip operations */
irq_ops_t irq_ops;

irqdesc_t *irq_to_desc(uint32_t irq)
{
	return &irqdesc[irq];
}

/*
 * Main thread entry point for deferred processing.
 * At the entry, IRQs are on.
 */
void *__irq_deferred_fn(void *args)
{
	int *ret;
	uint32_t irq = *((uint32_t *)args);

	while (atomic_read(&irqdesc[irq].deferred_pending)) {
		atomic_set(&irqdesc[irq].deferred_pending, 0);

		local_irq_enable();

		/* Perform the deferred processing bound to this IRQ */
		ret = (int *)irqdesc[irq].irq_deferred_fn(irq,
							  irqdesc[irq].data);

		local_irq_disable();

		/* At this point, we are coherent; if the same IRQ occurred before the end of this thread,
		 * the deferred_pending boolean is passed to true, and the while{} will be re-executed once more.
		 * If deferred_pending remains to false, and since IRQs are now disabled, the top half processing
		 * has not reached the assignment of deferred_pending to true, and therefore a new thread will
		 * be re-spawned. We are safe.
		 */
	}

	irqdesc[irq].thread_active = false;

	/* Re-enabling IRQs */
	local_irq_enable();

	/* Release the heap memory allocated for args */
	free(args);

	return ret;
}

/*
 * Process interrupt with top & bottom halves processing.
 */
void irq_process(uint32_t irq)
{
	int ret;
	char th_name[THREAD_NAME_LEN];
	int *args;

	if (boot_stage < BOOT_STAGE_IRQ_INIT)
		return; /* Ignore it */

	/* Immediate (top half) processing */

	if (irqdesc[irq].action != NULL)
		ret = irqdesc[irq].action(irq, irqdesc[irq].data);

	/*
	 * Deferred (bottom half) processing.
	 * A thread is created and started if it is the case.
	 */
	ASSERT(local_irq_is_disabled());

	if ((ret == IRQ_BOTTOM) && (irqdesc[irq].irq_deferred_fn != NULL)) {
		atomic_set(&irqdesc[irq].deferred_pending, 1);

		if (!irqdesc[irq].thread_active) {
			irqdesc[irq].thread_active = true;

			args = malloc(sizeof(uint32_t));
			BUG_ON(!args);

			memcpy(args, &irq, sizeof(uint32_t));

			sprintf(th_name, "irq_bottom/%d", irq);

			kernel_thread(__irq_deferred_fn, th_name, args, 0);
		}
	}
}

/**
 * @brief Attach a specific IRQ chip to a specific IRQ
 * 
 * @param irq 
 * @param irq_ops 
 */
void irq_set_irq_ops(int irq, irq_ops_t *irq_ops)
{
	irqdesc[irq].irq_ops = irq_ops;
}

/*
 * Bind a IRQ number with a specific top half handler and bottom half if any.
 */
void irq_bind(int irq, irq_handler_t handler, irq_handler_t irq_deferred_fn,
	      void *data)
{
	DBG("Binding irq %d with action at %x\n", irq, handler);

	BUG_ON(irqdesc[irq].action != NULL);

	irqdesc[irq].action = handler;
	irqdesc[irq].irq_deferred_fn = irq_deferred_fn;
	irqdesc[irq].data = data;

	irqdesc[irq].irq_ops->enable(irq);
}

void irq_unbind(int irq)
{
	DBG("Binding irq %d with action at %x\n", irq, handler);
	irqdesc[irq].action = NULL;
	irqdesc[irq].irq_deferred_fn = NULL;
}

void irq_mask(int irq)
{
	if (irq_to_desc(irq)->irq_ops->mask)
		irq_to_desc(irq)->irq_ops->mask(irq);
}

void irq_unmask(int irq)
{
	if (irq_to_desc(irq)->irq_ops->unmask)
		irq_to_desc(irq)->irq_ops->unmask(irq);
}

void irq_enable(int irq)
{
	if (irq_to_desc(irq)->irq_ops->enable)
		irq_to_desc(irq)->irq_ops->enable(irq);
	irq_unmask(irq);
}

void irq_disable(int irq)
{
	irq_mask(irq);
	if (irq_to_desc(irq)->irq_ops->disable)
		irq_to_desc(irq)->irq_ops->disable(irq);
}

void irq_handle(cpu_regs_t *regs)
{
	/* The following boolean indicates we are currently in the interrupt call path.
	 * It will be reset at the end of the softirq processing.
	 */

	__in_interrupt = true;

	irq_ops.handle_low(regs);

	/* Out of this interrupt routine, IRQs must be enabled otherwise the thread
	 * will block all interrupts.
	 */

#if !defined(CONFIG_AVZ) && !defined(CONFIG_SOO)

	/*
	 * We do not check if IRQs were disabled before the handler in case of an hypercall
	 * path which may happen in AVZ and in the ME because hypercalls are executed during
	 * their initialization along which a timer IRQ can be raised up and lead to their processing
	 * here.
	 */

	BUG_ON(irqs_disabled_flags(regs));
#endif

	do_softirq();
}

/*
 * Main interrupt device initialization function
 */
void irq_init(void)
{
	int i;

	memset(&irq_ops, 0, sizeof(irq_ops_t));

	irq_ops.handle_high = irq_process;

	for (i = 0; i < NR_IRQS; i++) {
		irqdesc[i].action = NULL;
		irqdesc[i].irq_deferred_fn = NULL;

		irqdesc[i].irq_ops = &irq_ops;

		atomic_set(&irqdesc[i].deferred_pending, 0);

		irqdesc[i].thread_active = false;
	}

	/* Initialize the softirq subsystem */
	softirq_init();
}
