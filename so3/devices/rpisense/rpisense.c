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
 */

#if 0
#define DEBUG
#endif

#include <common.h>
#include <memory.h>
#include <types.h>
#include <completion.h>

#include <asm/io.h>

#include <device/device.h>
#include <device/driver.h>
#include <device/irq.h>
#include <device/arch/i2c_bsc.h>
#include <device/i2c.h>

#include <device/arch/rpisense.h>

static uint32_t gpio_regs_addr;

uint8_t leds_array[SIZE_FB];

unsigned char matrix[SIZE_FB];

static joystick_handler_t __joystick_handler;
static void *__arg;


/* 5-bit per colour */
#define RED	{ 0x00, 0xf8 }
#define GREEN	{ 0xe0, 0x07 }
#define BLUE	{ 0x1f, 0x00 }
#define GRAY    { 0xef, 0x3d }

#define WHITE	{ 0xff, 0xff }
#define BLACK	{ 0x00, 0x00 }

unsigned char ledsoff[][2] = {
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
};

unsigned char leds[][64][2] = {
	{
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLUE, BLUE, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLUE, BLUE, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	},
	{
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	},
	{
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, GREEN, GREEN, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, GREEN, GREEN, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	},
	{
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, GRAY, GRAY, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, GRAY, GRAY, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	},
	{
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, RED, RED, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, RED, RED, BLACK, BLACK, BLACK,
		BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	}
};

void rpisense_matrix_display(u16 *mem, bool on) {
	int i, j;

	switch (on) {
	case true:
		for (j = 0; j < 8; j++) {
			for (i = 0; i < 8; i++) {
				matrix[(j * 24) + i + 1] |= (mem[(j * 8) + i] >> 11) & 0x1F;
				matrix[(j * 24) + (i + 8) + 1] |= (mem[(j * 8) + i] >> 6) & 0x1F;
				matrix[(j * 24) + (i + 16) + 1] |= mem[(j * 8) + i] & 0x1F;
			}
		}
		break;


	case false:
		for (j = 0; j < 8; j++) {
			for (i = 0; i < 8; i++) {
				matrix[(j * 24) + i + 1] &= ~((mem[(j * 8) + i] >> 11) & 0x1F);
				matrix[(j * 24) + (i + 8) + 1] &= ~((mem[(j * 8) + i] >> 6) & 0x1F);
				matrix[(j * 24) + (i + 16) + 1] &= ~(mem[(j * 8) + i] & 0x1F);
			}
		}
	}

	i2c_write(RPISENSE_I2C_ADDR, matrix, SIZE_FB);
}

void display_led(int led_nr, bool on) {
	u16 *mem = (u16 *) leds[led_nr];

	rpisense_matrix_display(mem, on);
}

void rpisense_matrix_off(void) {
	u16 *mem = (u16 *) ledsoff;
	rpisense_matrix_display(mem, false);
}

static irq_return_t joystick_interrupt_deferred(int irq, void *arg) {
		
	int key = 0;
	int prev_key = 0;

	i2c_read_smbus_data_byte(RPISENSE_I2C_ADDR, (uint8_t *) &key, JOYSTICK_ADDR);

	if (prev_key != key) {
		if (__joystick_handler) {
			__joystick_handler(__arg, key);
			prev_key = key;
		}
	}

	return IRQ_COMPLETED;
}

static irq_return_t joystick_interrupt_isr(int irq, void *arg) {
	
	/* Acknowledge the IRQ */
	iowrite32(gpio_regs_addr + GPIO_GPEDS0, JOYSTICK_GPIO);

	return IRQ_BOTTOM;
}

void rpisense_joystick_handler_register(void *arg, joystick_handler_t joystick_handler) {
	__joystick_handler = joystick_handler;
	__arg = arg;
}

static int rpisense_init(dev_t *dev, int fdt_offset) {
	uint32_t mask, fsel2;
	irq_def_t irq_def;

	fdt_interrupt_node(fdt_offset, &irq_def);

	gpio_regs_addr = io_map(GPIO_REGS_ADDR, 0xF0);

	mask = ioread32(gpio_regs_addr + GPIO_GPREN0);

	/* Enable GPIO23 Rising Edge detection */
	mask |= JOYSTICK_GPIO;
	iowrite32(gpio_regs_addr + GPIO_GPREN0, mask);

	/* Set GPIO23 as an input (it should be the default but apparently it is not) */
	fsel2 = ioread32(gpio_regs_addr + GPIO_GPFSEL2);
	fsel2 &= ~FSEL_INPUT_23;
	iowrite32(gpio_regs_addr + GPIO_GPFSEL2, fsel2);

	/* Bind the GPIO0 bank IRQ */
	irq_bind(irq_def.irqnr, joystick_interrupt_isr, joystick_interrupt_deferred, NULL);

	return 0;
}

REGISTER_DRIVER_CORE("bcm,rpisense", rpisense_init);
