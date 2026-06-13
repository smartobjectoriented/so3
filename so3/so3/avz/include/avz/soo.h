/*
 * Copyright (C) 2014-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <soo/uapi/soo.h>

/* Device tree features */
#define S3C_FEAT_ROOT "/s3c_features"

void soo_activity_init(void);
void shutdown_S3C(unsigned int S3C_slotID);

S3C_state_t get_S3C_state(uint32_t S3C_slotID);
void set_S3C_state(uint32_t slotID, S3C_state_t state);

#endif /* SOO_H */
