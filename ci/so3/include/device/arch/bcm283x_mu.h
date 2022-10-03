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

#ifndef BCM28x_MU_H
#define BCM28x_MU_H

#include <types.h>

#define UART_THR	0x0
#define UART_LSR	0x14

/* Bits and regs definitions (taken from U-boot) */
typedef struct {
	u32 io;
	u32 iir;
	u32 ier;
	u32 lcr;
	u32 mcr;
	u32 lsr;
	u32 msr;
	u32 scratch;
	u32 cntl;
	u32 stat;
	u32 baud;
} bcm283x_mu_t;

/* LSR register bits */
#define UART_LSR_RX_READY  (1 << 0)
#define UART_LSR_TX_READY  (1 << 5)

#endif /* BCM28x_MU_H */
