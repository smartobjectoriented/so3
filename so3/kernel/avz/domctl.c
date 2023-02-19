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

#if 0
#define DEBUG
#endif

#include <common.h>
#include <types.h>
#include <memory.h>
#include <errno.h>

#include <asm/processor.h>

#include <device/irq.h>

#include <avz/event.h>
#include <avz/domctl.h>
#include <avz/domain.h>
#include <avz/sched.h>
#include <avz/domctl.h>
#include <avz/debug.h>

static DEFINE_SPINLOCK(domctl_lock);

void do_domctl(domctl_t *args)
{
	struct domain *d;

	spin_lock(&domctl_lock);

	d = domains[args->domain];

	switch (args->cmd)
	{
	case DOMCTL_pauseME:

		domain_pause_by_systemcontroller(d);

		break;

	case DOMCTL_unpauseME:
		/* Retrieve info from hypercall parameter structure */
		d->avz_shared->vbstore_pfn = args->u.unpause_ME.vbstore_pfn;

		DBG("%s: unpausing ME\n", __func__);

		domain_unpause_by_systemcontroller(d);

		break;

#ifdef CONFIG_ARM64VT
	case DOMCTL_get_AVZ_shared:
		args->u.avz_shared_paddr =
			memslot[(current_domain->avz_shared->domID == DOMID_AGENCY) ? 1 : current_domain->avz_shared->domID].ipa_addr + memslot[MEMSLOT_AGENCY].size;
		break;
#endif /* CONFIG_ARM64VT */

	}

	spin_unlock(&domctl_lock);
}

