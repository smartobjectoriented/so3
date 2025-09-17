/*
 * Copyright (C) 2025 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
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

#include <soo/soo.h>
#include <soo/dev/vlogs.h>

static int logs_serial_put_byte(char c)
{
	if (vlogs_ready())
		vlogs_write(&c, 1);

	return 1;
}

/* Sends some bytes to the vlogs driver */
static int logs_serial_write(char *str, int len)
{
	int i;
	unsigned long flags;

	/* Here, we disable IRQ since printk() can also be used with IRQs off */
	flags = local_irq_save();

	for (i = 0; i < len; i++)
		if (str[i] != 0)
			logs_serial_put_byte(str[i]);

	local_irq_restore(flags);

	return len;
}

/*
 * Generates logs messages. It is similar 'printk', but:
 *   1. Add '[ME:<ME_ID>]' prefix
 *   2. Send message though vUART
 *
 *   Only available in virtual mode
 */
void logs(const char *fmt, ...)
{
	static char buf[1024];
	static char msg[1024];
	va_list args;
	char *p, *q;
	ME_desc_t *capsule_desc;

	va_start(args, fmt);
	(void) vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	capsule_desc = get_ME_desc();

	sprintf(msg, "[ME:%d] %s", capsule_desc->slotID, buf);

	p = msg;

	while ((q = strchr(p, '\n')) != NULL) {
		*q = '\0';

		logs_serial_write(p, strlen(p) + 1);
		logs_serial_write("\n", 2);

		p = q + 1;
	}

	if (*p != '\0')
		logs_serial_write(p, strlen(p) + 1);
}

static int logs_write(int fd, const void *buffer, int count)
{
	logs((char *) buffer);

	return count;
}

/* This structure will be used by vfs for initializing basic file descriptors
 * such as stdin, stdout, stderr.
 */
struct file_operations logs_fops = {
	.write = logs_write,
};

struct devclass logs_dev = {
	.class = "logs",
	.type = VFS_TYPE_DEV_CHAR,
	.fops = &logs_fops,
};

void vlogs_cdev_init(dev_t *dev)
{
	/* Register the mydev driver so it can be accessed from user space. */
	devclass_register(dev, &logs_dev);
}
