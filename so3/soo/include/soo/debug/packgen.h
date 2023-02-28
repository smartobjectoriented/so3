/*
 * Copyright (C) 2018 Baptiste Delporte <bonel@bonel.net>
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

#ifndef PACKGEN_H
#define PACKGEN_H

#ifdef __KERNEL__
#include <linux/types.h>
#endif /* __KERNEL__ */

#include <soo/debug/packcommon.h>

typedef struct {
	int (*send)(void *data, size_t size);
} packgen_callbacks_t;

void packgen_register(packgen_callbacks_t *callbacks, size_t size);
void packgen_enable_multi(unsigned int id);
void packgen_send_next_packet(void);

#endif /* PACKGEN_H */
