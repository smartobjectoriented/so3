/*
 * Copyright (C) 2014-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017 Alexandre Malki <alexandre.malki@heig-vd.ch>
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

#include <common.h>
#include <string.h>
#include <signal.h>
#include <mutex.h>
#include <process.h>

#include <device/irq.h>
#include <device/serial.h>

#include <asm/processor.h>
#include <asm/processor.h>

serial_ops_t serial_ops;
mutex_t read_lock;
tcb_t *tcb_owner;
volatile bool serial_intr = false;

/* Outputs an ASCII string to console */
int serial_putc(char c)
{
	if (serial_ops.put_byte == NULL) {
		__ll_put_byte(c);

		return 1;
	} else
		return serial_ops.put_byte(c);
}

/* Get a byte from the UART. */
char serial_getc(void)
{
	char c;

	/* This function will get a byte but first take try to
	 * take the lock to access the uart */

	mutex_lock(&read_lock);

	/* Keep a reference to the waiting thread which owns the serial device temporary. */
	tcb_owner = current();

	c = serial_ops.get_byte(false);

	mutex_unlock(&read_lock);

	return c;
}

/*
 * Emergency printk() or very early printk() before the real uart driver is ready.
 * It may be used in the context of the virtualized capsule SO3 as well.
 */
int ll_serial_write(char *str, int len)
{
	int i;

	for (i = 0; i < len; i++)
		if (str[i] != 0)
			__ll_put_byte(str[i]);

	return len;
}

/* Sends some bytes to the UART */
int serial_write(char *str, int len)
{
	int i;
	unsigned long flags;

	/* Here, we disable IRQ since printk() can also be used with IRQs off */
	flags = local_irq_save();

	for (i = 0; i < len; i++)
		if (str[i] != 0)
			serial_putc(str[i]);

	local_irq_restore(flags);

	return len;
}

/* This function will query the size of the serial terminal*/
int serial_gwinsize(struct winsize *wsz)
{
	wsz->ws_row = WINSIZE_ROW_SIZE_DEFAULT;
	wsz->ws_col = WINSIZE_COL_SIZE_DEFAULT;
	return 0;
}

#ifdef CONFIG_VIRT32
/*
 * Send a specific char code to Qemu to ask it to exit immediately.
 */
void send_qemu_halt(void)
{
	serial_write(SERIAL_SO3_HALT, 1);
}

#endif

/*
 * Release the lock if a process/thread is finishing during a read.
 */
void serial_cleanup(void)
{
	/* Check if the lock is owned by a thread which is currently terminated (via ctrl/c) or
	 * killed. In this case only, and if the thread is locking the serial device, the lock must be released.
	 */
	if ((current() == tcb_owner) && mutex_is_locked(&read_lock))
		mutex_unlock(&read_lock);
}

/*
 * Main initialization function of the UART device
 */
void serial_init(void)
{
	memset(&serial_ops, 0, sizeof(serial_ops_t));

	mutex_init(&read_lock);
}
