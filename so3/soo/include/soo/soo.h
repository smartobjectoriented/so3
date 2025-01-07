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

#ifndef SOO_H
#define SOO_H

#include <types.h>

#include <soo/uapi/soo.h>

int get_ME_state(void);
void set_ME_state(ME_state_t state);

int32_t get_ME_free_slot(uint32_t size);

bool get_ME_id(uint32_t slotID, ME_id_t *ME_id);

void get_ME_id_array(ME_id_t *ME_id_array);
char *xml_prepare_id_array(ME_id_t *ME_id_array);

ME_desc_t *get_ME_desc(void);

/* ME ID management */
const char *get_me_shortdesc(void);
const char *get_me_name(void);
u64 get_spid(void);

void vbstore_ME_ID_populate(void);

#endif /* SOO_H */
