/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef PRINTK_H
#define PRINTK_H

#include <stdarg.h>

void lprintk(char *format, ...);
void llprintk(char *format, ...);

void lprintk_buffer(void *buffer, uint32_t n);

void printk(const char *fmt, ...);

#ifndef __ASSEMBLY__

struct va_format {
	const char *fmt;
	va_list *va;
};

#define SOO_AGENCY_UID_SIZE		16

typedef struct {

	/*
	 * As id is the first attribute, it can be accessed directly by using
	 * a pointer to the agencyUID_t.
	 */
	unsigned char id[SOO_AGENCY_UID_SIZE];

} agencyUID_t;

/* Helper function to display agencyUID */
static inline void lprintk_printUID(agencyUID_t *uid) {

	/* Normally, the length of agencyUID is SOO_AGENCY_UID_SIZE bytes, but we display less. */
	if (!uid)
		lprintk("(null)");
	else
		lprintk_buffer(uid, 5);
}

/* Helper function to display agencyUID */
static inline void lprintk_printlnUID(agencyUID_t *uid) {
	lprintk_printUID(uid);
	lprintk("\n");
}


#endif /* !__ASSEMBLY__ */

#endif /* PRINTK_H */
