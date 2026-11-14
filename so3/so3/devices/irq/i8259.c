/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
/*
 * Legacy 8259A PIC pair, cascaded on IRQ 2: IRQ n is delivered on vector
 * X86_IRQ_BASE + n. Interrupts follow the 2-cell binding <irq type>.
 */

#include <common.h>
#include <spinlock.h>

#include <device/driver.h>
#include <device/irq.h>
#include <device/fdt.h>

#include <asm/io.h>
#include <asm/processor.h>

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xa0
#define PIC2_DATA 0xa1

#define PIC_EOI 0x20
#define PIC_READ_ISR 0x0b

#define ICW1_INIT 0x11
#define ICW4_8086 0x01

#define PIC_CASCADE_IRQ 2

static u16 pic_mask_cache = 0xffff;

static void pic_write_mask(void)
{
	outb(PIC1_DATA, pic_mask_cache & 0xff);
	outb(PIC2_DATA, pic_mask_cache >> 8);
}

static void pic_mask(unsigned int irq)
{
	pic_mask_cache |= (1 << irq);
	pic_write_mask();
}

static void pic_unmask(unsigned int irq)
{
	pic_mask_cache &= ~(1 << irq);

	/* The slave PIC needs the cascade line of the master */

	if (irq >= 8)
		pic_mask_cache &= ~(1 << PIC_CASCADE_IRQ);

	pic_write_mask();
}

static u16 pic_read_isr(void)
{
	outb(PIC1_CMD, PIC_READ_ISR);
	outb(PIC2_CMD, PIC_READ_ISR);

	return (inb(PIC2_CMD) << 8) | inb(PIC1_CMD);
}

static void pic_eoi(unsigned int irq)
{
	if (irq >= 8)
		outb(PIC2_CMD, PIC_EOI);

	outb(PIC1_CMD, PIC_EOI);
}

static void pic_handle(void *data)
{
	cpu_regs_t *regs = (cpu_regs_t *) data;
	unsigned int irq = regs->vector - X86_IRQ_BASE;

	/*
	 * IRQ 7 and 15 may be spurious: the line dropped before the ack. The
	 * in-service bit tells. A spurious IRQ 15 still needs the master's EOI.
	 */

	if ((irq == 7 || irq == 15) && !(pic_read_isr() & (1 << irq))) {
		if (irq == 15)
			outb(PIC1_CMD, PIC_EOI);
		return;
	}

	irq_to_desc(irq)->irq_ops->handle_high(irq);

	pic_eoi(irq);
}

void fdt_interrupt_node(int fdt_offset, irq_def_t *irq_def)
{
	const struct fdt_property *prop;
	const fdt32_t *p;
	int prop_len;

	prop = fdt_get_property(__fdt_addr, fdt_offset, "interrupts", &prop_len);
	BUG_ON(!prop);

	p = (const fdt32_t *) prop->data;

	BUG_ON(prop_len != 2 * sizeof(uint32_t));

	irq_def->irq_class = 0;
	irq_def->irqnr = fdt32_to_cpu(p[0]);
	irq_def->irq_type = fdt32_to_cpu(p[1]);
}

static int i8259_init(dev_t *dev, int fdt_offset)
{
	/* Remap both PICs above the CPU exceptions */

	outb(PIC1_CMD, ICW1_INIT);
	io_delay();
	outb(PIC2_CMD, ICW1_INIT);
	io_delay();
	outb(PIC1_DATA, X86_IRQ_BASE);
	io_delay();
	outb(PIC2_DATA, X86_IRQ_BASE + 8);
	io_delay();
	outb(PIC1_DATA, 1 << PIC_CASCADE_IRQ);
	io_delay();
	outb(PIC2_DATA, PIC_CASCADE_IRQ);
	io_delay();
	outb(PIC1_DATA, ICW4_8086);
	io_delay();
	outb(PIC2_DATA, ICW4_8086);
	io_delay();

	pic_mask_cache = 0xffff;
	pic_write_mask();

	irq_ops.enable = pic_unmask;
	irq_ops.disable = pic_mask;
	irq_ops.mask = pic_mask;
	irq_ops.unmask = pic_unmask;
	irq_ops.handle_low = pic_handle;

	return 0;
}

REGISTER_DRIVER_CORE("intel,8259", i8259_init);
