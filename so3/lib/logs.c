/*
 * Copyright (C) 2024 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
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

#include <string.h>
#include <common.h>
#include <stdarg.h>
#include <process.h>
#include <vfs.h>
#include <device/serial.h>

#include <soo/avz.h>

/* Sends some bytes to the UART */
static int log_write(char *str, int len)
{
	int i;
	unsigned long flags;

	/* Here, we disable IRQ since printk() can also be used with IRQs off */
	flags = local_irq_save();

	for (i = 0; i < len; i++) {
		if (str[i] != 0)
			serial_putc(str[i]);
	}

	local_irq_restore(flags);

	return len;
}

/*
 * Generates logs messages. It is similar to 'printk', but:
 *   1. Prefixes '[ME:<ME_ID>] to the message
 *   2. Sends message through vUART
 *
 *   Only available in ME mode (when SO3 is used as a ME)
 */
void logs(const char *fmt, ...)
{
	static char buf[1024];
	static char msg[1024];
	va_list args;
	char *p, *q;

	va_start(args, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	sprintf(msg, "[ME:%d] %s", ME_domID(), buf);

	p = msg;

	while ((q = strchr(p, '\n')) != NULL) {
		*q = '\0';

		log_write(p, strlen(p) + 1);
		log_write("\n", 2);

		p = q + 1;
	}

	if (*p != '\0')
		log_write(p, strlen(p) + 1);
}
