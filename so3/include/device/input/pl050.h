/*
 * Copyright (C) 2020 Nikolaos Garanis <nikolaos.garanis@heig-vd.ch>
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

#include <device/irq.h>


/* Register address offsets */
#define KMI_CR     0x00
#define KMI_STAT   0x04
#define KMI_DATA   0x08
#define KMI_CLKDIV 0x0c
#define KMI_IR     0x10

/* Control register values */
#define KMICR_TYPE     (0 << 5) /* PS2/AT mode */
#define KMICR_RXINTREN (1 << 4) /* enable receiver interrupt */
#define KMICR_TXINTREN (0 << 3) /* disable transmitter interrupt */
#define KMICR_EN       (1 << 2) /* enable interface */
#define KMICR_FD       (0 << 1)
#define KMICR_FC       (0 << 0)

/* Status register values */
#define KMISTAT_TXEMPTY  (1 << 6) /* transmit register is empty, can be written */
#define KMISTAT_TXBUSY   (1 << 5) /* data is being sent */
#define KMISTAT_RXFULL   (1 << 4) /* receive register is full, can be read */
#define KMISTAT_RXBUSY   (1 << 3) /* data is being received */
#define KMISTAT_RXPARITY (1 << 2) /* parity of last received byte */
#define KMISTAT_IC       (1 << 1)
#define KMISTAT_ID       (1 << 0)

/* Interrupt status register values */
#define KMIIR_TXINTR (1 << 1) /* transmitter interrupt asserted */
#define KMIIR_RXINTR (1 << 0) /* receiver interrupt asserted */

void pl050_write(void *base, uint8_t data);
void pl050_init(void *base, irq_def_t *irq_def, irq_return_t (*isr)(int, void *));
