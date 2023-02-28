/*
 * Copyright (C) 2018-2019 Daniel Rossier <daniel.rossier@soo.tech>
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
#include <process.h>
#include <vfs.h>

#include <soo/hypervisor.h>
#include <soo/console.h>

extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

void lprintk(char *format, ...) {
	char buf[CONSOLEIO_BUFFER_SIZE];
	int i;
	va_list va;
	uint32_t flags;

	va_start(va, format);
	vsnprintf(buf, CONSOLEIO_BUFFER_SIZE, format, va);
	va_end(va);

	flags = local_irq_save();

	BUG_ON(strlen(buf) > CONSOLEIO_BUFFER_SIZE);

	for (i = 0; i < strlen(buf); i++)
		__printch(buf[i]);

	local_irq_restore(flags);

}

/**
 * Print the contents of a buffer.
 */
void lprintk_buffer(void *buffer, uint32_t n) {
	uint32_t i;

	for (i = 0 ; i < n ; i++)
		lprintk("%02x ", ((char *) buffer)[i]);
}

/**
 * Print the contents of a buffer. Each element is separated using a given character.
 */
void lprintk_buffer_separator(void *buffer, uint32_t n, char separator) {
	uint32_t i;

	for (i = 0 ; i < n ; i++)
		lprintk("%02x%c", ((char *) buffer)[i], separator);
}

/**
 * Print an uint64_t number and concatenate a string.
 */
void lprintk_int64_post(s64 number, char *post) {
	uint32_t msb = number >> 32;
	uint32_t lsb = number & 0xffffffff;

	lprintk("%08x %08x%s", msb, lsb, post);
}

/**
 * Print an uint64_t number.
 */
void lprintk_int64(s64 number) {
	lprintk_int64_post(number, "\n");
}
