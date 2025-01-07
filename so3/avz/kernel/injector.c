/*
 * Copyright (C) 2014-2022 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#if 1
#define DEBUG
#endif

#include <heap.h>
#include <memory.h>
#include <crc.h>

#include <avz/memslot.h>
#include <avz/domain.h>
#include <avz/sched.h>
#include <avz/injector.h>

#include <soo/uapi/soo.h>

#include <asm/cacheflush.h>

#include <libfdt/image.h>

#include <libfdt/libfdt.h>

/**
 *  Inject a ME within a SOO device. This is the only possibility to load a ME within a Smart Object.
 *
 *  At the entry of this function, the ME ITB could have been allocated in the user space (via the injector application)
 *  or in the vmalloc'd area of the Linux kernel in case of a BT transfer from the tablet (using vuihandler).
 *
 *  To get rid of the way how the page tables are managed by Linux, we perform a copy of the ME ITB in the
 *  AVZ heap, assuming that the 8-MB heap is sufficient to host the ITB ME (< 2 MB in most cases).
 *
 *  If the ITB should become larger, it is still possible to compress (and enhance AVZ with a uncompressor invoked
 *  at loading time). Wouldn't be still not enough, a temporary fixmap mapping combined with get_free_pages should be envisaged
 *  to have the ME ITB accessible from the AVZ user space area.
 *
 * @param op  (op->vaddr is the ITB buffer, op->p_val1 will contain the slodID in return (-1 if no space), op->p_val2 is the ITB buffer size_
 */

void inject_me(avz_hyp_t *args)
{
	int slotID;
	size_t fdt_size;
	void *fdt_vaddr;
        struct domain *domME, *__current;
        unsigned long flags;
        void *itb_vaddr;
        mem_info_t guest_mem_info;

        DBG("%s: Preparing ME injection, source image vaddr = %lx\n", __func__, 
		ipa_to_va(MEMSLOT_AGENCY, args->u.avz_inject_me_args.itb_paddr));

	flags = local_irq_save();

	itb_vaddr = (void *) ipa_to_va(MEMSLOT_AGENCY, args->u.avz_inject_me_args.itb_paddr);

        DBG("%s: ITB vaddr: %lx\n", __func__, itb_vaddr);

	/* Retrieve the domain size of this ME through its device tree. */
	fit_image_get_data_and_size(itb_vaddr, fit_image_get_node(itb_vaddr, "fdt"), (const void **) &fdt_vaddr, &fdt_size);
	if (!fdt_vaddr) {
		printk("### %s: wrong device tree.\n", __func__);
		BUG();
	}

        get_mem_info(fdt_vaddr, &guest_mem_info);

        /* Find a slotID to store this ME. */
        slotID = get_ME_free_slot(guest_mem_info.size, ME_state_booting);
        if (slotID == -1)
		goto out;

	domME = domains[slotID];

	/* Set the size of this ME in its own descriptor */
	domME->avz_shared->dom_desc.u.ME.size = memslot[slotID].size;

	/* Now set the pfn base of this ME; this will be useful for the Agency Core subsystem */
	domME->avz_shared->dom_desc.u.ME.pfn = phys_to_pfn(memslot[slotID].base_paddr);

	__current = current_domain;

	/* Clear the RAM allocated to this ME */
	memset((void *) __xva(slotID, memslot[slotID].base_paddr), 0, memslot[slotID].size);

	loadME(slotID, itb_vaddr);

	if (construct_ME(domains[slotID]) != 0)
		panic("Could not set up ME guest OS\n");

out:
	/* Prepare to return the slotID to the caller. */
	args->u.avz_inject_me_args.slotID = slotID;
 
	local_irq_restore(flags);
}
