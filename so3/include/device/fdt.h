#ifndef FDT_H
#define FDT_H
/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 * Copyright 2012 Kim Phillips, Freescale Semiconductor.
 *
 * libfdt is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 *
 *  a) This library is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation; either version 2 of the
 *     License, or (at your option) any later version.
 *
 *     This library is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public
 *     License along with this library; if not, write to the Free
 *     Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 *     MA 02110-1301 USA
 *
 * Alternatively,
 *
 *  b) Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *     1. Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *     2. Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *     CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *     NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *     OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *     EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <compiler.h>

#include <libfdt/libfdt.h>

#define MAX_COMPAT_SIZE		128
#define MAX_NODE_SIZE 		128
#define MAX_SUBNODE		4

/* Reference to the current device tree */
extern void *__fdt_addr;

int fdt_find_compatible_node(void *fdt_addr, char *compat);
const struct fdt_property *fdt_find_property(void *fdt_addr, int offset, const char *propname);
int fdt_find_node_by_name(void *fdt_addr, int parent, const char *nodename);

int fdt_property_read_string(void *fdt_addr, int offset, const char *propname, const char **out_string);
int fdt_property_read_u32(void *fdt_addr, int offset, const char *propname, u32 *out_value);
int fdt_property_read_u64(void *fdt_addr, int offset, const char *propname, u64 *out_value);

int fdt_pack_reg(const void *fdt, void *buf, addr_t *address, size_t *size);
int fdt_find_or_add_subnode(void *fdt, int parentoffset, const char *name);

/*
 * Get device information from a device tree
 * This function will be in charge of allocating dev_inf struct;
 */
int get_dev_info(const void *fdt_addr, int offset, const char *compat, void *info);
int fdt_get_int(void *fdt_addr, void *dev, const char *name);
bool fdt_device_is_available(void *fdt_addr, int node_offset);
void *find_device(const char *compat);

#endif /* FDT_H */
