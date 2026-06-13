/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef __AVZCONS_H__
#define __AVZCONS_H__

void avzcons_force_flush(void);
void avzcons_resume(void);

/* Interrupt work hooks. Receive data, or kick data out. */
void avzcons_rx(char *buf, unsigned len, struct pt_regs *regs);
void avzcons_tx(void);

int avzcons_ring_init(void);
int avzcons_ring_send(const char *data, unsigned len);

int avz_switch_console(char ch);

#endif /* __AVZCONS_H__ */
