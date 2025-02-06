
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

#include <soo/console.h>
#include <soo/evtchn.h>
#include <soo/hypervisor.h>
#include <soo/physdev.h>

/* When the heap is not available yet, we use this temporary buffer
 * to pass the hypervisor args.
 */
avz_hyp_t __avz_hyp_static;

void avz_get_shared(void)
{
	avz_hyp_t args;

	args.cmd = AVZ_DOMAIN_CONTROL_OP;
	args.u.avz_domctl_args.domctl.cmd = DOMCTL_get_AVZ_shared;

	avz_hypercall(&args);

	BUG_ON(!args.u.avz_domctl_args.domctl.avz_shared_paddr);

	avz_shared = (avz_shared_t *)io_map(
		args.u.avz_domctl_args.domctl.avz_shared_paddr, PAGE_SIZE);
	BUG_ON(!avz_shared);
}

void avz_printch(char c)
{
	avz_hyp_t args;

	args.cmd = AVZ_CONSOLE_IO_OP;

	args.u.avz_console_io_args.console.cmd = CONSOLE_IO_PRINTCH;
	args.u.avz_console_io_args.console.u.c = c;

	avz_hypercall(&args);
}

void avz_printstr(char *s)
{
	avz_hyp_t args;

	args.cmd = AVZ_CONSOLE_IO_OP;
	args.u.avz_console_io_args.console.cmd = CONSOLE_IO_PRINTSTR;

	strncpy(args.u.avz_console_io_args.console.u.str, s,
		CONSOLE_STR_MAX_LEN);

	avz_hypercall(&args);
}

void avz_gnttab(gnttab_op_t *op)
{
	avz_hyp_t args;

	args.cmd = AVZ_GRANT_TABLE_OP;

	memcpy(&args.u.avz_gnttab_args.gnttab_op, op, sizeof(gnttab_op_t));
	avz_hypercall(&args);
	memcpy(op, &args.u.avz_gnttab_args.gnttab_op, sizeof(gnttab_op_t));
}

/*
 * SOO hypercall
 *
 * Mandatory arguments:
 * - cmd: hypercall
 * - addr: a virtual address used within the hypervisor
 * - p_val1: a (virtual) address to a first value
 * - p_val2: a (virtual) address to a second value
 */

void avz_hypercall(avz_hyp_t *avz_hyp)
{
	avz_hyp_t *__avz_hyp;

	if (boot_stage < BOOT_STAGE_HEAP_READY) {
		__avz_hyp = &__avz_hyp_static;
	} else {
		/* Make sure the avz_hyp details are in a linear-mapped zone
                * to be able to pass the physical address to the hypervisor.
                */
		__avz_hyp = malloc(sizeof(avz_hyp_t));
		BUG_ON(!__avz_hyp);
	}

	memcpy(__avz_hyp, avz_hyp, sizeof(avz_hyp_t));

	__asm_flush_dcache_range((addr_t)__avz_hyp, sizeof(avz_hyp_t));
	__avz_hypercall(AVZ_HYPERCALL_TRAP, __pa(__avz_hyp));
	__asm_invalidate_dcache_range((addr_t)__avz_hyp, sizeof(avz_hyp_t));

	memcpy(avz_hyp, __avz_hyp, sizeof(avz_hyp_t));

	if (boot_stage > BOOT_STAGE_HEAP_READY)
		free(__avz_hyp);
}

void avz_sig_terminate(void)
{
	__avz_hypercall(AVZ_HYPERCALL_SIGRETURN, 0);
}