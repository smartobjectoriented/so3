/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2021 Nicolas Müller <nicolas.muller1@heig-vd.ch>
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

#define UART_CLK_FREQ		24000000 /* 24 MHz */
#define UART_BAUDRATE		115200

/* Bits and regs definitions
 * Note: On the current version of u-boot for RISC-V on qemu, CONFIG_DM_SERIAL is active. Addresses
 * for register access are on 1 byte because of it. Since u-boot configures low level UART,
 * kernel must be adapted to it */
#define UART_RBR_REG_OFFSET	        0
#define UART_DLL_REG_OFFSET	        0
#define UART_DLH_REG_OFFSET	        1
#define UART_IER_REG_OFFSET	        1
#define UART_IIR_REG_OFFSET	        2
#define UART_FCR_REG_OFFSET	        2
#define UART_LCR_REG_OFFSET	        3
#define UART_MCR_REG_OFFSET	        4
#define UART_LSR_REG_OFFSET	        5
#define UART_MSR_REG_OFFSET	        6
#define UART_SCH_REG_OFFSET	        7

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
