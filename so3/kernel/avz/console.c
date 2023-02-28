/*
 * Copyright (C) 2016,2017 Daniel Rossier <daniel.rossier@soo.tech>
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
#include <stdarg.h>
#include <errno.h>
#include <spinlock.h>
#include <console.h>
#include <serial.h>
#include <softirq.h>
#include <memory.h>
#include <types.h>
#include <string.h>
#include <version.h>

#include <device/serial.h>

#include <avz/console.h>
#include <avz/keyhandler.h>
#include <avz/event.h>

#include <asm/io.h>

DEFINE_SPINLOCK(console_lock);

/* To manage the final virtual address of the UART */
extern addr_t __uart_vaddr;

void serial_puts(const char *s)
{
	char c;

	while ((c = *s++) != '\0')
		printch(c);
}

/*
 * Remap the UART which is currently identity-mapped so that all I/O are in the hypervisor area.
 */
void console_init_post(void)
{
	__uart_vaddr = (addr_t) io_map(CONFIG_UART_LL_PADDR, PAGE_SIZE);
	BUG_ON(!__uart_vaddr);
}

static void sercon_puts(const char *s)
{
	serial_puts(s);
}

/* (DRE) Perform hypercall to process char addressed to the keyhandler mechanism */
void process_char(char ch) {
	handle_keypress(ch);
}

void do_console_io(int cmd, int count, char *buffer)
{
	char kbuf;

	switch (cmd) {

	case CONSOLEIO_process_char:
		memcpy(&kbuf, buffer, sizeof(char));

		process_char(kbuf);
		break;

	case CONSOLEIO_write_string:

		printk("%s", buffer);
		break;

	default:
		BUG();
	}
}


/*
 * *****************************************************
 * *************** GENERIC CONSOLE I/O *****************
 * *****************************************************
 */

static void __putstr(const char *str)
{
	sercon_puts(str);
}

/* The console_lock must be hold. */
static void __printk(char *buf) {
	char *p, *q;

	p = buf;

	while ((q = strchr(p, '\n')) != NULL)
	{
		*q = '\0';
		__putstr(p);
		__putstr("\n");
		p = q + 1;
	}

	if (*p != '\0')
		__putstr(p);
}

void printk(const char *fmt, ...)
{
	static char   buf[1024];
	va_list       args;

	spin_lock(&console_lock);

	va_start(args, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	__printk(buf);

	spin_unlock(&console_lock);
}

/* Just to keep compatibility with uapi/soo in Linux */
void lprintk(char *fmt, ...) {
	static char   buf[1024];
	va_list       args;

	spin_lock(&console_lock);

	va_start(args, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	__printk(buf);

	spin_unlock(&console_lock);
}


/**
 * Print the contents of a buffer.
 */
void printk_buffer(void *buffer, uint32_t n)
{
	uint32_t i;

	for (i = 0 ; i < n ; i++)
		printk("%02x ", ((char *) buffer)[i]);
	printk("\n");
}

void printk_buffer_separator(void *buffer, int n, char separator) {
	int i;

	for (i = 0 ; i < n ; i++)
		printk("%02x%c", ((char *) buffer)[i], separator);
	printk("\n");
}
