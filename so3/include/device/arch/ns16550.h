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

/*
 * uart.h
 *
 * Declarations of constants (UART base addr, etc.)
 */

#ifndef NS16550_H
#define NS16550_H

#include <types.h>

#define UART_THR			0x0
#define UART_LSR			0x14

#define UART_CLK_FREQ		24000000 /* 24 MHz */
#define UART_BAUDRATE		115200

/* Bits and regs definitions */
typedef struct {
	/* 0x00 */
	union {
		volatile uint32_t rbr;
		volatile uint32_t thr;
		volatile uint32_t dll;
	};
	/* 0x04 */
	union {
		volatile uint32_t dlh;
		volatile uint32_t ier;
	};
	/* 0x08 */
	union {
		volatile uint32_t iir;
		volatile uint32_t fcr;
	};

	volatile uint32_t lcr;          /* 0x0C */
	volatile uint32_t mcr;          /* 0x10 */
	volatile uint32_t lsr;          /* 0x14 */
	volatile uint32_t msr;          /* 0x18 */
	volatile uint32_t sch;          /* 0x1C */
	volatile uint32_t _res0[23];    /* 0x20-0x78 */
	volatile uint32_t usr;          /* 0x7C */
	volatile uint32_t tfl;          /* 0x80 */
	volatile uint32_t rfl;          /* 0x84 */
	volatile uint32_t _res1[7];     /* 0x88-0xA0 */
	volatile uint32_t halt;         /* 0xA4 */
} ns16550_t;

/* LCR register bits */
#define UART_5BITS      0x0
#define UART_6BITS      0x1
#define UART_7BITS      0x2
#define UART_8BITS      0x3

#define UART_1_STOP     (0 << 2)
#define UART_1_5_STOP   (1 << 2)

#define UART_PARITY_EN  (0 << 3)
#define UART_PARITY_DIS (1 << 3)

#define UART_LCR_DLAB   (1 << 7)

/* MCR register bits */
#define UART_DTR        (1 << 0)
#define UART_RTS        (1 << 1)

/* LSR register bits */
#define UART_LSR_DR     (1 << 0)
#define UART_LSR_THRE   (1 << 5)

#endif /* NS16550_H */
