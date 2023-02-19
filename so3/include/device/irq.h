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

#ifndef IRQ_H
#define IRQ_H

#include <spinlock.h>
#include <common.h>
#include <thread.h>
#include <percpu.h>

#include <asm/atomic.h>

/* Maximum physical interrupts than can be managed by SO3 */
#define NR_IRQS 		160

DECLARE_PER_CPU(spinlock_t, intc_lock);

typedef enum {
	IRQ_TYPE_NONE		= 0x00000000,
	IRQ_TYPE_EDGE_RISING	= 0x00000001,
	IRQ_TYPE_EDGE_FALLING	= 0x00000002,
	IRQ_TYPE_EDGE_BOTH	= (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING),
	IRQ_TYPE_LEVEL_HIGH	= 0x00000004,
	IRQ_TYPE_LEVEL_LOW	= 0x00000008,
	IRQ_TYPE_LEVEL_MASK	= (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH),
	IRQ_TYPE_SENSE_MASK	= 0x0000000f,
	IRQ_TYPE_DEFAULT	= IRQ_TYPE_SENSE_MASK,
} irq_type_t;

typedef enum {
	IRQ_COMPLETED = 0,
	IRQ_BOTTOM
} irq_return_t;

typedef struct {
	int irqnr;
	int irq_class;
	int irq_type;
} irq_def_t;

typedef irq_return_t(*irq_handler_t)(int irq, void *data);

typedef struct irqdesc {
	irq_handler_t action;

	/* Deferred action */
	irq_handler_t irq_deferred_fn;
	atomic_t deferred_pending;
	bool thread_active;

	/* Private data */
	void *data;

	/* Multi-processing scenarios */
	spinlock_t lock;

} irqdesc_t;

extern volatile bool __in_interrupt;

extern int arch_irq_init(void);
extern void setup_arch(void);

/* IRQ controller */
typedef struct  {

    void (*enable)(unsigned int irq);
    void (*disable)(unsigned int irq);
    void (*mask)(unsigned int irq);
    void (*unmask)(unsigned int irq);

    void (*handle)(cpu_regs_t *regs);

} irq_ops_t;

extern irq_ops_t irq_ops;

int irq_process(uint32_t irq);

void irq_init(void);

irqdesc_t *irq_to_desc(uint32_t irq);

void irq_mask(int irq);
void irq_unmask(int irq);

void irq_bind(int irq, irq_handler_t handler, irq_handler_t irq_deferred_fn, void *data);
void irq_unbind(int irq);

void fdt_interrupt_node(int fdt_offset, irq_def_t *irq_def);

#endif /* IRQ_H */
