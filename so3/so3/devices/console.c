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

#include <termios.h>
#include <device/serial.h>

/* Default termios flags that will be used by the console. */
static termios_t termios = {
	.c_iflag = INLCR,
	.c_lflag = ECHO | ICANON,
};

static int console_write(int fd, const void *buffer, int count);

static char console_get_next_c(void)
{
	char c = serial_getc();

	/* Convert CR to LF if required */
	if (termios.c_iflag & INLCR) {
		if (c == '\r')
			c = '\n';
	}

	return c;
}

static void icanon_handle_char(int fd, char c, char *buf, size_t *total)
{
	/* Backspace handle */
	if (c == 127) {
		if (*total != 0) {
			/* Delete last char from console and buffer */
			buf[*total] = '\0';
			(*total)--;

			if (termios.c_lflag & ECHO)
				console_write(fd, "\b \b", 3);
		}

		return;
	}

	/* Generic handle */
	buf[*total] = c;
	(*total)++;

	if (termios.c_lflag & ECHO)
		console_write(fd, &c, 1);
}

/* Used to read from a serial (uart) console. We report only one byte when the byte is ready. */
static int console_getc(int fd, void *buffer, int count)
{
	char c;
	char *c_buf = (char *) buffer;
	size_t total = 0;

	if (termios.c_lflag & ICANON) {
		do {
			/* Read and handle next byte  */
			c = console_get_next_c();
			icanon_handle_char(fd, c, c_buf, &total);
		} while (c != '\n' && total < count);
	} else {
		*c_buf = console_get_next_c();
		total = 1;
	}

	return total;
}

/* Send out to the serial console. */
static int console_write(int fd, const void *buffer, int count)
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
	case TCGETS:
		*(struct termios *) args = termios;
		rc = 0;
		break;

	case TCSETS:
	case TCSETSW:
	case TCSETSF:
		/* We should normally wait for all output to be trasmitted
		   and flush input depending on the IOCTL. But we will assumed
		   this will already by ok for now. */
		termios = *(struct termios *) args;
		rc = 0;
		break;

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
