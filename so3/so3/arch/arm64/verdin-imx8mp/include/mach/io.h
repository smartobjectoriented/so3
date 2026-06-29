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

#ifndef MACH_IO_H
#define MACH_IO_H

/*
 * Freescale i.MX8MP UART3 register offsets (fsl,imx6q-uart compatible).
 * Base address: 0x30880000 (UART3, used as debug console).
 */

#define IMX_UART_URXD 0x00 /* Receiver register */
#define IMX_UART_UTXD 0x40 /* Transmitter register */
#define IMX_UART_UCR1 0x80 /* Control register 1 */
#define IMX_UART_UCR2 0x84 /* Control register 2 */
#define IMX_UART_UCR3 0x88 /* Control register 3 */
#define IMX_UART_UCR4 0x8c /* Control register 4 */
#define IMX_UART_UFCR 0x90 /* FIFO control register */
#define IMX_UART_USR1 0x94 /* Status register 1 */
#define IMX_UART_USR2 0x98 /* Status register 2 */

/* USR2 bits */
#define IMX_UART_USR2_TXFE (1 << 14) /* TX FIFO empty */
#define IMX_UART_USR2_TXDC (1 << 3) /* Transmitter complete */

/* UCR1 bits */
#define IMX_UART_UCR1_UARTEN (1 << 0) /* UART enable */

/* UCR2 bits */
#define IMX_UART_UCR2_TXEN (1 << 2) /* Transmitter enable */
#define IMX_UART_UCR2_RXEN (1 << 1) /* Receiver enable */
#define IMX_UART_UCR2_SRST (1 << 0) /* SW reset (active low) */

#endif /* MACH_IO_H */
