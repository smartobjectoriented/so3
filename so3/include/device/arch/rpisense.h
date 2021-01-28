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

#ifndef RPISENSE_H
#define RPISENSE_H

#include <types.h>

#define GPIO_GPFSEL2	0x08
#define GPIO_GPREN0	0x4c
#define GPIO_GPEDS0	0x40
#define GPIO_GPLEV0	0x34

#define FSEL_INPUT_23	0x00000e00

#define NB_ROW 8
#define NB_COL 8

/* 8 (row) * 8 (column) * 3 (colors) + 1 (first byte needed blank) */
#define SIZE_FB 193

#define UP      0x04
#define DOWN    0x01
#define RIGHT   0x02
#define LEFT    0x10
#define CENTER  0x08

#define RPISENSE_I2C_ADDR	0x46

#define JOYSTICK_GPIO		(1 << 23)
#define JOYSTICK_ADDR		0xf2

typedef void(*joystick_handler_t)(void *arg, int key);

void display_led(int led_nr, bool on);
void rpisense_matrix_off(void);

void rpisense_joystick_handler_register(void *arg, joystick_handler_t joystick_handler);

#endif /* RPISENSE_H */
