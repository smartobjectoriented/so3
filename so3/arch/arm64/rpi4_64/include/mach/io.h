/*
 * Copyright (C) 2020-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
 * From BCM2835 ARM peripherals documentation.
 *
 * Mini UART register offsets.
 */

#define MU_IO_REG 		0x00
#define MU_IER_REG 		0x04
#define MU_IIR_REG 		0x08
#define MU_LCR_REG 		0x0C
#define MU_MCR_REG 		0x10
#define MU_LSR_REG 		0x14
#define MU_MSR_REG 		0x18
#define MU_SCRATCH 		0x1C
#define MU_CNTL_REG 		0x20
#define MU_STAT_REG 		0x24
#define MU_BAUD_REG 		0x28

#define MU_IO_DATA 		(0xFF << 0)
#define MU_STAT_SP_AVAIL	 (1 << 0)
#define MU_STAT_RX_FIFO_FILL 	(0xF << 16)
#define MU_LSR_TX_EMPTY 	(1 << 5)
#define MU_LSR_DATA_READY 	(1 << 0)
/* TODO : all other bit offsets */


/*
 * The low 4 bits of this are the CPU's per-mailbox IRQ enables, and
 * the next 4 bits are the CPU's per-mailbox FIQ enables (which
 * override the IRQ bits).
 */
#define LOCAL_MAILBOX_INT_CONTROL0	0x050

/*
 * Mailbox write-to-set bits.  There are 16 mailboxes, 4 per CPU, and
 * these bits are organized by mailbox number and then CPU number.  We
 * use mailbox 0 for IPIs.  The mailbox's interrupt is raised while
 * any bit is set.
 */
#define LOCAL_MAILBOX0_SET0		0x080
#define LOCAL_MAILBOX3_SET0		0x08c

/* Mailbox write-to-clear bits. */
#define LOCAL_MAILBOX0_CLR0		0x0c0
#define LOCAL_MAILBOX3_CLR0		0x0cc

/* CPU Release address to be woken up following spin table method. */
#define CPU0_RELEASE_ADDR	0xd8
#define CPU1_RELEASE_ADDR	0xe0
#define CPU2_RELEASE_ADDR	0xe8
#define CPU3_RELEASE_ADDR	0xf0

#endif /* MACH_IO_H */

