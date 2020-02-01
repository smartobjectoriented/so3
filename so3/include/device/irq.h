/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <common.h>
#include <thread.h>

#include <asm/atomic.h>

#include <device/fdt/fdt.h>

/* Maximum physical interrupts than can be managed by SO3 */
#define NR_IRQS 		128

typedef enum {
	IRQ_COMPLETED = 0,
	IRQ_BOTTOM
} irq_return_t;

typedef irq_return_t(*irq_handler_t)(int irq, void *data);

typedef struct irqdesc {
	irq_handler_t action;

	/* Deferred action */
	irq_handler_t irq_deferred_fn;
	atomic_t deferred_pending;
	bool thread_active;

	/* Private data */
	void *data;

} irqdesc_t;

extern volatile bool __in_interrupt;

extern int arch_irq_init(void);
extern void setup_arch(void);

/* IRQ controller */
typedef struct  {

    void (*irq_enable)(unsigned int irq);
    void (*irq_disable)(unsigned int irq);
    void (*irq_mask)(unsigned int irq);
    void (*irq_unmask)(unsigned int irq);

    void (*irq_handle)(cpu_regs_t *regs);

} irq_ops_t;

extern irq_ops_t irq_ops;

int irq_process(uint32_t irq);

void irq_init(void);

irqdesc_t *irq_to_desc(uint32_t irq);

void irq_mask(int irq);
void irq_unmask(int irq);

void irq_bind(int irq, irq_handler_t handler, irq_handler_t irq_deferred_fn, void *data);
void irq_unbind(int irq);

#endif /* IRQ_H */
