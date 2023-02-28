/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2016-2018 Baptiste Delporte <bonel@bonel.net>
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

#ifndef CONSOLE_H
#define CONSOLE_H

#include <types.h>
#include <soo/soo.h>

#define CONSOLEIO_BUFFER_SIZE 256

struct pt_regs;

void avzcons_rx(char *buf, unsigned len, struct pt_regs *regs);
void avzcons_tx(void);

void init_console(void);

extern void (*__printch)(char c);

void lprintk(char *format, ...);
void lprintk_buffer(void *buffer, uint32_t n);
void lprintk_buffer_separator(void *buffer, uint32_t n, char separator);

void lprintk_int64_post(s64 number, char *post);
void lprintk_int64(s64 number);

/* Helper function to display agencyUID */
static inline void lprintk_printUID(uint64_t uid) {
	int i;
	uint8_t *c = (uint8_t *) &uid;

	if (!uid)
		lprintk("n/a");
	else
		for (i = 0; i < 8; i++ ) {
			lprintk("%02x ", *(c+7-i)); /* Display byte per byte */
		}
}

/* Helper function to display agencyUID */
static inline void lprintk_printlnUID(uint64_t uid) {
	lprintk_printUID(uid);
	lprintk("\n");
}

/* Return the current active domain for the uart console */
int avzcons_get_focus(void);

/* Set the active domain to next_domain for the uart console and return the current active domain. */
int avzcons_set_focus(int next_domain);

#define avzcons_NEXT_FOCUS()	(avzcons_set_focus((avzcons_get_focus() + 1) % 4))

/* Return the current active domain for the graphic console */
int vfb_get_focus(void);

/* Set the active domain to next_domain for the graphic console and return the current active domain. */
int vfb_set_focus(int next_domain);

#define VFB_NEXT_FOCUS()	(vfb_set_focus((vfb_get_focus() + 1) % 3))

/* Temporary helper until the vfbback interaction have been cleared up */
#define VFB_TRIG_IRQ() \
	({\
		extern irqreturn_t vfb_interrupt(int irq, void *dev_id); \
	   	vfb_interrupt(0, NULL); \
	})

#endif /* CONSOLE_H */
