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

#define UART_THR 0x0
#define UART_LSR 0x14

/*
 * Bits and regs definitions.
 *
 * NOTE: this layout used to be copied from U-Boot's struct bcm283x_mu_regs,
 * which lists iir before ier. That is wrong: the mini UART is 8250-compatible
 * (Linux drives it with the generic 8250 driver, and the BCM2835 peripherals
 * doc puts AUX_MU_IER_REG at +0x04 and AUX_MU_IIR_REG at +0x08, as the
 * MU_IER_REG/MU_IIR_REG offsets in mach/io.h already stated). The mistake was
 * harmless as long as neither field was touched — U-Boot never reads them and
 * this driver only used io and lsr — but it bites the moment ier is written to
 * enable the RX interrupt.
 */
typedef struct {
	u32 io; /* 0x00 */
	u32 ier; /* 0x04 */
	u32 iir; /* 0x08 */
	u32 lcr; /* 0x0c */
	u32 mcr; /* 0x10 */
	u32 lsr; /* 0x14 */
	u32 msr; /* 0x18 */
	u32 scratch; /* 0x1c */
	u32 cntl; /* 0x20 */
	u32 stat; /* 0x24 */
	u32 baud; /* 0x28 */
} bcm283x_mu_t;

/* LSR register bits */
#define UART_LSR_RX_READY (1 << 0)
#define UART_LSR_TX_READY (1 << 5)

/* IER register bits (8250 semantics, see the note above) */
#define UART_IER_RX_ENABLE (1 << 0)
#define UART_IER_TX_ENABLE (1 << 1)

#endif /* BCM28x_MU_H */
