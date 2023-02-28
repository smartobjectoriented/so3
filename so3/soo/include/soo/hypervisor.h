/*
 * Copyright (C) 2016-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef _HYPERVISOR_H_
#define _HYPERVISOR_H_

#include <soo/avz.h>
#include <soo/physdev.h>

void domcall(int cmd, void *arg);

void spad_enable_cooperate(void);
void spad_disable_cooperate(void);

int do_presetup_adjust_variables(void *arg);
int do_postsetup_adjust_variables(void *arg);
int do_sync_domain_interactions(void *arg);
int do_sync_directcomm(void *arg);

void inject_syscall_vector(void);
void avz_vector_callback(void);

#endif /* __HYPERVISOR_H__ */
