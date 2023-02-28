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

#ifndef PACKCOMMON_H
#define PACKCOMMON_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/string.h>
#else
#include <core/types.h>
#include <stddef.h>
#endif /* __KERNEL__ */

#define PACKGENCHK_N_PREDEF_PACKETS		16
#define PACKGENCHK_MAX_SOOS			16

#ifdef __KERNEL__
#define packcommon_alloc(size) kzalloc(size, GFP_KERNEL)
#else
#define packcommon_alloc(size) malloc(size)
#endif /* __KERNEL__ */

void pack_prepare_predef_packets(unsigned char *predef_packets[PACKGENCHK_N_PREDEF_PACKETS], size_t size);
void pack_patch_predef_packets(unsigned char *predef_packets[PACKGENCHK_N_PREDEF_PACKETS], unsigned char *prefix, size_t prefix_size);

#endif /* PACKCOMMON_H */
