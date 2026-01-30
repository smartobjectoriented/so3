/*
 * Copyright (C) 2014-2025 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#define ME_FEAT_ROOT "/me_features"

void soo_activity_init(void);
void shutdown_ME(unsigned int ME_slotID);

ME_state_t get_ME_state(uint32_t ME_slotID);
void set_ME_state(uint32_t slotID, ME_state_t state);

#endif /* SOO_H */
