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

#include <types.h>

#define ioread8(p)      (*((volatile uint8_t *) (p)))
#define ioread16(p)     (*((volatile uint16_t *) (p)))
#define ioread32(p)     (*((volatile uint32_t *) (p)))
#define ioread64(p)     (*((volatile uint64_t *) (p)))

#define iowrite8(p,v)   (*((volatile uint8_t *) (p)) = v)
#define iowrite16(p,v)  (*((volatile uint16_t *) (p)) = v)
#define iowrite32(p,v)  (*((volatile uint32_t *) (p)) = v)
#define iowrite64(p,v)  (*((volatile uint64_t *) (p)) = v)
