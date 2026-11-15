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
 * 16550 UART in the x86 I/O port space (COM1 at 0x3f8 on a PC). The device
 * tree gives the port base in "reg". Reception is interrupt-driven, as for
 * the PL011.
 */

#include <types.h>
#include <process.h>
#include <signal.h>
#include <mutex.h>
#include <schedule.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/serial.h>
#include <device/irq.h>
#include <device/fdt.h>

#include <asm/io.h>

#define UART_RBR 0 /* Receive buffer (DLAB = 0) */
#define UART_THR 0 /* Transmit holding (DLAB = 0) */
#define UART_DLL 0 /* Divisor low (DLAB = 1) */
#define UART_IER 1 /* Interrupt enable (DLAB = 0) */
#define UART_DLH 1 /* Divisor high (DLAB = 1) */
#define UART_FCR 2 /* FIFO control */
#define UART_LCR 3 /* Line control */
#define UART_MCR 4 /* Modem control */
#define UART_LSR 5 /* Line status */

#define UART_IER_RDI 0x01
#define UART_LCR_DLAB 0x80
#define UART_LCR_8N1 0x03
#define UART_MCR_DTR_RTS_OUT2 0x0b
#define UART_FCR_ENABLE_CLEAR 0x07
#define UART_LSR_DR 0x01
#define UART_LSR_THRE 0x20

/* 115200 bauds from the 1.8432 MHz clock */

#define UART_DIVISOR 1

#define SERIAL_BUFFER_SIZE 80

extern mutex_t read_lock;

static u16 uart_port = CONFIG_UART_LL_PORT;
static irq_def_t uart_irq;

static volatile char serial_buffer[SERIAL_BUFFER_SIZE];
static volatile uint32_t prod = 0, cons = 0;

static int uart_put_byte(char c)
{
	while (!(inb(uart_port + UART_LSR) & UART_LSR_THRE))
		;

	outb(uart_port + UART_THR, c);

	return 1;
}

static char uart_get_byte(bool polling)
{
	char tmp;

	if (polling) {
		while (!(inb(uart_port + UART_LSR) & UART_LSR_DR))
			;

		return inb(uart_port + UART_RBR);
	}

	while (prod == cons) {
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

/* Ctrl-C is turned into SIGINT for the foreground process, as on the PL011 */

static void uart_rx_char(char c)
{
	if (c == 3) {
		uart_put_byte('^');
		uart_put_byte('C');
		uart_put_byte('\n');

#ifdef CONFIG_IPC_SIGNAL
		if (mutex_is_locked(&read_lock)) {
			serial_intr = true;
		} else {
			pcb_t *target = fg_pcb ? fg_pcb : current()->pcb;

			if (target != NULL)
				sys_do_kill(target->pid, SIGINT);
		}
#endif
		return;
	}

	serial_buffer[prod] = c;
	prod = (prod + 1) % SERIAL_BUFFER_SIZE;
}

static irq_return_t uart_int(int irq, void *dummy)
{
	while (inb(uart_port + UART_LSR) & UART_LSR_DR)
		uart_rx_char(inb(uart_port + UART_RBR));

	return IRQ_COMPLETED;
}

static void uart_enable_irq(void)
{
	irq_ops.enable(uart_irq.irqnr);
}

static void uart_disable_irq(void)
{
	irq_ops.disable(uart_irq.irqnr);
}

static int uart_pio_init(dev_t *dev, int fdt_offset)
{
	const struct fdt_property *prop;
	int prop_len;

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);
	BUG_ON(prop_len != 2 * sizeof(u64));

	uart_port = fdt64_to_cpu(((const fdt64_t *) prop->data)[0]);

	serial_ops.put_byte = uart_put_byte;
	serial_ops.get_byte = uart_get_byte;
	serial_ops.enable_irq = uart_enable_irq;
	serial_ops.disable_irq = uart_disable_irq;

	outb(uart_port + UART_IER, 0);

	outb(uart_port + UART_LCR, UART_LCR_DLAB);
	outb(uart_port + UART_DLL, UART_DIVISOR & 0xff);
	outb(uart_port + UART_DLH, UART_DIVISOR >> 8);
	outb(uart_port + UART_LCR, UART_LCR_8N1);

	outb(uart_port + UART_FCR, UART_FCR_ENABLE_CLEAR);

	/* OUT2 gates the UART interrupt line on a PC */

	outb(uart_port + UART_MCR, UART_MCR_DTR_RTS_OUT2);

	fdt_interrupt_node(fdt_offset, &uart_irq);
	irq_bind(uart_irq.irqnr, uart_int, NULL, NULL);

	outb(uart_port + UART_IER, UART_IER_RDI);

	return 0;
}

void __ll_put_byte(char c)
{
	uart_put_byte(c);
}

void printch(char c)
{
	__ll_put_byte(c);
}

REGISTER_DRIVER_POSTCORE("x86,uart-pio", uart_pio_init);
