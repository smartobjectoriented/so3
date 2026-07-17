/*
 * Copyright (C) 2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <heap.h>
#include <limits.h>
#include <memory.h>
#include <mutex.h>
#include <process.h>
#include <signal.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>

#include <device/serial.h>

#include <device/arch/bcm283x_mu.h>
#include <mach/io.h>

#include <asm/io.h> /* ioread/iowrite macros */

#define SERIAL_BUFFER_SIZE 80

void *__uart_vaddr = (void *) CONFIG_UART_LL_PADDR;

static volatile char serial_buffer[SERIAL_BUFFER_SIZE];
static volatile uint32_t prod = 0, cons = 0;

extern mutex_t read_lock;

typedef struct {
	addr_t base;
	irq_def_t irq_def;
	bool irq_enabled;
} bcm283x_mu_dev_t;

static bcm283x_mu_dev_t bcm283x_mu_dev = {
	.base = CONFIG_UART_LL_PADDR,
};

static int bcm283x_mu_put_byte(char c)
{
	bcm283x_mu_t *bcm283x_mu = (bcm283x_mu_t *) bcm283x_mu_dev.base;

	/* Wait until there is space in the FIFO */
	while (!(ioread32(&bcm283x_mu->lsr) & UART_LSR_TX_READY))
		;

	/* Send the character */
	iowrite32(&bcm283x_mu->io, (uint32_t) c);

	if (c == '\n') {
		while (!(ioread32(&bcm283x_mu->lsr) & UART_LSR_TX_READY))
			;
		iowrite8(&bcm283x_mu->io, '\r'); /* Carriage return */
	}

	return 0;
}

void printch(char c)
{
	bcm283x_mu_put_byte(c);
}

void __ll_put_byte(char c)
{
	bcm283x_mu_put_byte(c);
}

static char bcm283x_mu_get_byte(bool polling)
{
	bcm283x_mu_t *bcm283x_mu = (bcm283x_mu_t *) bcm283x_mu_dev.base;
	char tmp;

	/* Without an RX interrupt there is no producer filling the ring, so the
	 * only thing we can do is spin on the FIFO — the behaviour this driver
	 * had before interrupts were wired up.
	 */
	if (polling || !bcm283x_mu_dev.irq_enabled) {
		while (!(ioread32(&bcm283x_mu->lsr) & UART_LSR_RX_READY))
			;

		return (char) ioread32(&bcm283x_mu->io);
	}

	while (prod == cons) {
		/* Ctrl-C arrived while we were blocked reading: abandon the
		 * read and report ETX so console_getc cancels the line. */
		if (serial_intr) {
			serial_intr = false;
			return 3; /* ETX */
		}

		schedule();

		smp_mb();
		wfi();
	}

	tmp = serial_buffer[cons];
	cons = (cons + 1) % SERIAL_BUFFER_SIZE;

	return tmp;
}

/*
 * Drain the RX FIFO into the serial buffer. Reading MU_IO is what clears the
 * interrupt, so we loop on LSR rather than decoding IIR — that also sidesteps
 * the mini UART's muddled IIR documentation.
 *
 * Ctrl-C handling mirrors pl011_int(): if a thread holds read_lock it is
 * blocked reading the console (e.g. the shell), so we only cancel its line
 * instead of killing it; otherwise a foreground app is running and gets SIGINT.
 */
static irq_return_t bcm283x_mu_int(int irq, void *dummy)
{
	bcm283x_mu_t *bcm283x_mu = (bcm283x_mu_t *) bcm283x_mu_dev.base;
	char c;

	while (ioread32(&bcm283x_mu->lsr) & UART_LSR_RX_READY) {
		c = (char) ioread32(&bcm283x_mu->io);

		/* Check for SIGINT to be raised on Ctrl^C */
		if (c == 3) {
			bcm283x_mu_put_byte('^');
			bcm283x_mu_put_byte('C');
			bcm283x_mu_put_byte('\n');

#ifdef CONFIG_IPC_SIGNAL
			if (mutex_is_locked(&read_lock)) {
				serial_intr = true;
			} else {
				/* Use fg_pcb (the shell's foreground job), NOT
				 * current(): in IRQ context current() is just the
				 * thread running when the key arrived (usually the
				 * idle thread, since the foreground app is typically
				 * asleep). Fall back to current() if no foreground is
				 * tracked yet. */
				pcb_t *target = fg_pcb ? fg_pcb : current()->pcb;
				if (target != NULL)
					sys_do_kill(target->pid, SIGINT);
			}
#endif
			/* Already echoed above; keep it out of the ring. */
			continue;
		}

		serial_buffer[prod] = c;
		prod = (prod + 1) % SERIAL_BUFFER_SIZE;
	}

	return IRQ_COMPLETED;
}

static void bcm283x_mu_enable_irq(void)
{
	irq_ops.enable(bcm283x_mu_dev.irq_def.irqnr);
}

static void bcm283x_mu_disable_irq(void)
{
	irq_ops.disable(bcm283x_mu_dev.irq_def.irqnr);
}

static int bcm283x_mu_init(dev_t *dev, int fdt_offset)
{
	const struct fdt_property *prop;
	int prop_len;
	addr_t new_base_vaddr;
	bcm283x_mu_t *bcm283x_mu;

	/* Pins multiplexing skipped here for simplicity (done by bootloader) */
	/* Clocks init skipped here for simplicity (done by bootloader) */

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);

	BUG_ON(prop_len != 2 * sizeof(unsigned long));

	/* Keep the boot-time UART base usable until io_map() has returned, so
	 * that io_map() itself can still print. */
#ifdef CONFIG_ARCH_ARM32
	new_base_vaddr =
		io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]), fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
#else
	new_base_vaddr =
		io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]), fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));
#endif
	BUG_ON(!new_base_vaddr);

	/* Initialize UART controller */
	memset(&bcm283x_mu_dev, 0, sizeof(bcm283x_mu_dev_t));
	bcm283x_mu_dev.base = new_base_vaddr;

	serial_ops.put_byte = bcm283x_mu_put_byte;
	serial_ops.get_byte = bcm283x_mu_get_byte;

	/* The interrupt is optional: fdt_interrupt_node() would BUG_ON a
	 * missing "interrupts", and a device tree that does not declare one
	 * simply keeps the polled console this driver started out with. Ctrl-C
	 * then cannot work, since nothing scans the RX line asynchronously.
	 */
	prop = fdt_get_property(__fdt_addr, fdt_offset, "interrupts", &prop_len);
	if (!prop)
		return 0;

	fdt_interrupt_node(fdt_offset, &bcm283x_mu_dev.irq_def);

#ifndef CONFIG_AVZ
	/* Same reasoning as pl011_init(): under AVZ the RX interrupt belongs to
	 * the agency guest, whose own console driver attaches a handler. Binding
	 * one here would let gic_handle() consume and EOI the IRQ before the
	 * guest ever sees it. AVZ only needs put_byte for output.
	 */
	irq_bind(bcm283x_mu_dev.irq_def.irqnr, bcm283x_mu_int, NULL, NULL);
#endif

	serial_ops.enable_irq = bcm283x_mu_enable_irq;
	serial_ops.disable_irq = bcm283x_mu_disable_irq;

	/* Enable RX interrupts at the controller, then at the GIC. */
	bcm283x_mu = (bcm283x_mu_t *) bcm283x_mu_dev.base;
	iowrite32(&bcm283x_mu->ier, UART_IER_RX_ENABLE);

	serial_ops.enable_irq();

	bcm283x_mu_dev.irq_enabled = true;

	return 0;
}

REGISTER_DRIVER_POSTCORE("serial,bcm283x-mu", bcm283x_mu_init);
