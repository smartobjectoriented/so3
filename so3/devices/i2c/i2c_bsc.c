/*
 * Copyright (C) 2020 David Truan <david.truan@heig-vd.ch>
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
 * I2C Broadcom Serial Controller (BSC)
 * Heavily inspired by https://github.com/raspberrypi/linux/blob/rpi-5.4.y/drivers/i2c/busses/i2c-bcm2835.c
 *
 */

#if 0
#define DEBUG
#endif

#include <common.h>
#include <memory.h>
#include <types.h>
#include <completion.h>
#include <mutex.h>

#include <asm/io.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>
#include <device/i2c.h>

#include <device/arch/i2c_bsc.h>

static struct mutex bus_lock;

typedef struct {
	struct i2c_regs *base;
	irq_def_t irq_def;
} i2c_t;

i2c_t i2c;

typedef struct i2c_msg {
	/* Slave address */
	uint16_t addr;

	/* Message length */
	uint16_t len;

	/* Buffer address */
	uint8_t *buf;

	/* Bytes remaining to read/write */
	uint32_t remaining;

	/* Indicates if this is a reading or writing transfer */
	bool is_read;
} i2c_msg_t;

static i2c_msg_t cur_msg;

/* Used to wait for the end of the transfer */
static completion_t transfer_done_completion;

void i2c_bsc_fill_fifo(void) {
	uint32_t val;
	while (cur_msg.remaining != 0) {

		val = ioread32(&i2c.base->i2c_stat);
		if (!(val & S_TXD))
			break;

		iowrite32(&(i2c.base->i2c_fifo), (uint32_t) (*cur_msg.buf & 0x000000FF));

		cur_msg.buf++;
		cur_msg.remaining--;
	}
}

void i2c_bsc_drain_fifo(void) {
	uint32_t val;

	while (cur_msg.remaining != 0) {

		val = ioread32(&(i2c.base->i2c_stat));
		if (!(val & S_RXD))
			break;

		*cur_msg.buf = (uint8_t) ioread32(&i2c.base->i2c_fifo);
		cur_msg.buf++;
		cur_msg.remaining--;
	}
}

/* Configure the CTRL register and start the transfer */
static void i2c_bsc_start_transfer(bool is_read) {
	uint32_t c;

	/* Slave address and data length setup */
	iowrite32(&i2c.base->i2c_dlen, cur_msg.len);
	iowrite32(&i2c.base->i2c_a, cur_msg.addr);

	/* Enable interrupts and I2C controller */
	c = CTRL_ST | CTRL_I2CEN | CTRL_INTD;

	/* Select reading/writing */
	if (is_read) {
		c |= CTRL_READ | CTRL_INTR;
	} else {
		c &= ~CTRL_READ;
		c |= CTRL_INTT;
	}
	iowrite32(&i2c.base->i2c_ctrl, c);

}

/* Message cleanup */
static void i2c_bsc_finish_transfer(void) {

	cur_msg.addr = 0;
	cur_msg.buf = NULL;
	cur_msg.len = 0;
	cur_msg.remaining = 0;

}

/*
 * ISR called by the I2C interrupt. It can be of 3 causes:
 *  -The transfer is DONE
 *  -The FIFO needs reading
 *  -The FIFO needs writing
 */
static irq_return_t i2c_bsc_isr(int irq, void *dummy) {
	uint32_t s;

	s = ioread32(&i2c.base->i2c_stat);

	if (s & S_DONE) {
		if (cur_msg.is_read == I2C_READ) {
			i2c_bsc_drain_fifo();
		}
		goto complete;
	}

	if (s & S_TXW) {
		if (cur_msg.remaining == 0) {
			goto complete;
		}
		i2c_bsc_fill_fifo();
	}

	if (s & S_RXR) {
		if (cur_msg.remaining == 0) {
			goto complete;
		}
		i2c_bsc_drain_fifo();
	}

	return IRQ_COMPLETED;

complete:

	iowrite32(&i2c.base->i2c_ctrl, CTRL_CLEAR_FIFO);
	iowrite32(&i2c.base->i2c_stat, S_ERR | S_CLKT | S_DONE);

	complete(&transfer_done_completion);

	return IRQ_COMPLETED;
}

/* Setups the message and the start/wait for the end of the transfer */
static void i2c_xfer(uint32_t slave_addr, uint8_t *buf, size_t size, bool reading) {

	mutex_lock(&bus_lock);

	cur_msg.addr = slave_addr;
	cur_msg.buf = buf;
	cur_msg.len = size;
	cur_msg.remaining = size;
	cur_msg.is_read = reading;

	i2c_bsc_start_transfer(reading);

	wait_for_completion(&transfer_done_completion);

	i2c_bsc_finish_transfer();

	mutex_unlock(&bus_lock);
}

void i2c_bsc_read_data(uint32_t slave_addr, uint8_t *buf, size_t size) {
	i2c_xfer(slave_addr, buf, size, I2C_READ);
}

void i2c_bsc_write_data(uint32_t slave_addr, uint8_t *buf, size_t size) {
	i2c_xfer(slave_addr, buf, size, I2C_WRITE);
}

static int i2c_bsc_init(dev_t *dev, int fdt_offset)
{
	const struct fdt_property *prop;
	int prop_len;
	uint32_t mask;
	uint32_t gpio_regs_addr;

	DBG("I2C broadcom - Initialization\n");

	memset(&i2c, 0, sizeof(i2c_t));

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);
	BUG_ON(prop_len != 2 * sizeof(unsigned long));

	/* Mapping the device properly */
#ifdef CONFIG_ARCH_ARM32
	i2c.base = (void *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]), fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
#else
	i2c.base = (void *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]), fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));

#endif
	fdt_interrupt_node(fdt_offset, &i2c.irq_def);

	/* Configuration of GPIOs 2 & 3 dedicated to I2C */
	gpio_regs_addr = io_map(GPIO_REGS_ADDR, 0xF0);
	mask = ioread32(gpio_regs_addr + GPIO_FSEL0_OFF);
	mask &= ~GPIO23_CLEAR;
	mask |= GPIO23_ALT0;

	/* Configuration GPIO 2 et 3 alt0 */
	iowrite32(gpio_regs_addr+GPIO_FSEL0_OFF, mask);

	/* Reset du registre ctrl l'i2c */
	iowrite32(&(i2c.base->i2c_ctrl), 0x0);

	/* Setup clock divider : 150 MHz / 1500 (0x5dc) = 100 kHz */
	iowrite32(&i2c.base->i2c_div, 0x5dc);

	/* Setup clock stretch timeout: NO_STRETCH */
	iowrite32(&i2c.base->i2c_clkt, NO_STRETCH);

	/* Binding with the I2C framework */
	i2c_ops.i2c_write_op = i2c_bsc_write_data;
	i2c_ops.i2c_read_op = i2c_bsc_read_data;

	mutex_init(&bus_lock);

	init_completion(&transfer_done_completion);

	irq_bind(i2c.irq_def.irqnr, i2c_bsc_isr, NULL, NULL);

	io_unmap(gpio_regs_addr);

	DBG("i2c_init end\n");

	return 0;
}

REGISTER_DRIVER_CORE("bcm,i2c_bsc", i2c_bsc_init);
