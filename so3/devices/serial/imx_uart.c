/*
 * Copyright (C) 2026 EDGEMTech SA <info@edgemtech.ch>
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
 * Minimal driver for the Freescale i.MX UART (fsl,imx6q-uart compatible).
 * Targets UART3 on the Toradex Verdin iMX8MP @ 0x30880000.
 *
 * Only TX is implemented — sufficient for AVZ's early console output.
 * UART initialisation is performed by ATF/U-Boot before AVZ starts,
 * so this driver assumes the UART is already configured and enabled.
 */

#include <types.h>
#include <heap.h>
#include <process.h>
#include <signal.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/serial.h>
#include <device/irq.h>

#include <asm/io.h>

#include <mach/io.h>

#include <printk.h>

#define SERIAL_BUFFER_SIZE 80

volatile void *__uart_vaddr = (void *) CONFIG_UART_LL_PADDR;

static volatile char serial_buffer[SERIAL_BUFFER_SIZE];
static volatile uint32_t prod = 0, cons = 0;

extern mutex_t read_lock;

typedef struct {
	addr_t base;
	irq_def_t irq_def;
} imx_uart_t;

static imx_uart_t imx_uart = {
	.base = CONFIG_UART_LL_PADDR,
};

static int imx_uart_put_byte(char c)
{
	/* Wait until TX FIFO is empty before writing */
	while (!(ioread32(imx_uart.base + IMX_UART_USR2) & IMX_UART_USR2_TXFE))
		;

	iowrite32(imx_uart.base + IMX_UART_UTXD, (u32) c);

	return 1;
}

static char imx_uart_get_byte(bool polling)
{
	char tmp;

	if (polling) {
		/* Poll until a byte arrives in the RX register */
		while (!(ioread32(imx_uart.base + IMX_UART_USR2) & (1 << 0)))
			;

		return (char) (ioread32(imx_uart.base + IMX_UART_URXD) & 0xFF);
	} else {
		while (prod == cons) {
			schedule();

			smp_mb();
			wfi();
		}

		tmp = serial_buffer[cons];
		cons = (cons + 1) % SERIAL_BUFFER_SIZE;

		return tmp;
	}

	return 0;
}

static irq_return_t imx_uart_int(int irq, void *dummy)
{
	u32 val;

	val = ioread32(imx_uart.base + IMX_UART_URXD);
	if (!(val & (1 << 15))) /* CHARRDY bit */
		return IRQ_COMPLETED;

	serial_buffer[prod] = (char) (val & 0xFF);

	if (serial_buffer[prod] == 3) {
		imx_uart_put_byte('^');
		imx_uart_put_byte('C');
		imx_uart_put_byte('\n');
		prod--;

#ifdef CONFIG_IPC_SIGNAL
		if (current()->pcb != NULL)
			sys_do_kill(current()->pcb->pid, SIGINT);
#endif
	}

	prod = (prod + 1) % SERIAL_BUFFER_SIZE;

	return IRQ_COMPLETED;
}

static void imx_uart_enable_irq(void)
{
	irq_ops.enable(imx_uart.irq_def.irqnr);
}

static void imx_uart_disable_irq(void)
{
	irq_ops.disable(imx_uart.irq_def.irqnr);
}

static int imx_uart_init(dev_t *dev, int fdt_offset)
{
	const struct fdt_property *prop;
	int prop_len;
	addr_t new_base_vaddr;

	serial_ops.put_byte = imx_uart_put_byte;
	serial_ops.get_byte = imx_uart_get_byte;

	serial_ops.enable_irq = imx_uart_enable_irq;
	serial_ops.disable_irq = imx_uart_disable_irq;

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);

	BUG_ON(prop_len != 2 * sizeof(unsigned long));

	new_base_vaddr = io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]), PAGE_SIZE);
	BUG_ON(!new_base_vaddr);

	imx_uart.base = new_base_vaddr;

	fdt_interrupt_node(fdt_offset, &imx_uart.irq_def);

	irq_bind(imx_uart.irq_def.irqnr, imx_uart_int, NULL, NULL);

	serial_ops.enable_irq();

	return 0;
}

void __ll_put_byte(char c)
{
	if (c == '\n')
		imx_uart_put_byte('\r');
	imx_uart_put_byte(c);
}

void printch(char c)
{
	__ll_put_byte(c);
}

REGISTER_DRIVER_POSTCORE("serial,imx", imx_uart_init);
