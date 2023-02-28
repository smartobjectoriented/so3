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

#ifndef PACKCHK_H
#define PACKCHK_H

#ifdef __KERNEL__
#include <linux/types.h>
#endif /* __KERNEL__ */

#include <soo/debug/packcommon.h>

typedef struct {
	int (*recv_success)(void *buffer, size_t size);
	int (*recv_failure)(void *buffer, size_t size);
} packchk_callbacks_t;

void packchk_register(packchk_callbacks_t *callbacks, size_t size);
void packchk_enable_multi(void);
void packchk_send_next_packet(void);
void packchk_process_received_packet(void *data, size_t size);

#endif /* PACKCHK_H */
