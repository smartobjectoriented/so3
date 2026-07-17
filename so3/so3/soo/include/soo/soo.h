/*
 * Copyright (C) 2016-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

int get_S3C_state(void);
void set_S3C_state(S3C_state_t state);

bool get_S3C_id(uint32_t slotID, S3C_id_t *S3C_id);

void get_S3C_id_array(S3C_id_t *S3C_id_array);
char *xml_prepare_id_array(S3C_id_t *S3C_id_array);

S3C_desc_t *get_S3C_desc(void);

/* capsule ID management */
const char *get_s3c_shortdesc(void);
const char *get_s3c_name(void);
u64 get_spid(void);

void vbstore_S3C_ID_populate(void);

#endif /* SOO_H */
