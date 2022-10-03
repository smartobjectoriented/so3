/*
 * Copyright (C) 2020 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <device/serial.h>

/* Used to read from a serial (uart) console. We report only one byte when the byte is ready. */
static int console_getc(int gfd, void *buffer, int count)
{
	/* Read one byte from the UART console */
	*((uint8_t *) buffer) = serial_getc();

	return 1;
}

/* Send out to the serial console. */
static int console_write(int gfd, const void *buffer, int count)
{
	int ret;

	ret = serial_write((char *) buffer, count);

	return ret;
}

/* Request an ioctl from user space */
static int console_ioctl(int fd, unsigned long cmd, unsigned long args)
{
	int rc;

	switch (cmd) {
		case TIOCGWINSZ:
			rc = serial_gwinsize((struct winsize *) args);
			break;
		default:
			rc = -1;
			break;
	}

	return rc;
}

/* This structure will be used by vfs for initializing basic file descriptors
 * such as stdin, stdout, stderr.
 */
struct file_operations console_fops = {
	.read = console_getc,
	.write = console_write,
	.ioctl = console_ioctl,
};


