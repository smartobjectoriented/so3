/*
 * Copyright (C) 2014-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef SERIAL_H
#define SERIAL_H

#include <device/device.h>

/* Serial IOCTL  */
#define TIOCGWINSZ	0x5413

/*  This is a reserved char code we use to query (patched) Qemu to retrieve the window size. */
#define SERIAL_GWINSZ	"\254"

/* The following code is used for telling Qemu that SO3 is aborting its execution.
 * This is used for terminating qemu properly when codecheck is executed.
 * qemu-system-arm must be started with option --codecheck
 */
#define SERIAL_SO3_HALT	"\253"

#define WINSIZE_ROW_SIZE_DEFAULT	25
#define WINSIZE_COL_SIZE_DEFAULT	80

typedef struct {
	int (*put_byte)(char c);
	char (*get_byte)(bool polling);
	void (*enable_irq)(void);
	void (*disable_irq)(void);
} serial_ops_t;

extern serial_ops_t serial_ops;

struct winsize {
	unsigned short	ws_row;		/* rows, in characters */
	unsigned short	ws_col;		/* columns, in characters */
	unsigned short	ws_xpixel;	/* horizontal size, pixels */
	unsigned short	ws_ypixel;	/* vertical size, pixels */
};

int serial_putc(char c);
char serial_getc(void);

int serial_write(char *str, int max);
int serial_read(char *buf, int len);
int serial_gwinsize(struct winsize *wsz);

int ll_serial_write(char *str, int len);

void serial_init(void);
void serial_cleanup(void);

void enable_uart_irq(void);
void disable_uart_irq(void);

extern void __ll_put_byte(char c);

#endif /* SERIAL_H */
