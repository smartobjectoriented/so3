
/*
 * Copyright (C) 2014-2024 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <bitops.h>
#include <heap.h>
 
#include <avz/uapi/avz.h>
#include <avz/uapi/domctl.h>

#include <soo/console.h>
#include <soo/evtchn.h>
#include <soo/hypervisor.h>
#include <soo/physdev.h>

void avz_get_shared(void) {
        struct domctl *op;

        op = malloc(sizeof(struct domctl));
        BUG_ON(!op);

        op->cmd = DOMCTL_get_AVZ_shared;

        __asm_flush_dcache_range((addr_t) op, sizeof(struct domctl));
        avz_hypercall(__HYPERVISOR_domctl, __pa(op), 0, 0, 0);
        __asm_invalidate_dcache_range((addr_t) op, sizeof(struct domctl));

        BUG_ON(!op->u.avz_shared_paddr);

        avz_shared = (avz_shared_t *) io_map(op->u.avz_shared_paddr, PAGE_SIZE);
        BUG_ON(!avz_shared);

        free(op);
}

void avz_printch(char c) {
        avz_hypercall(__HYPERVISOR_console_io, c, 0, 0, 0);
}