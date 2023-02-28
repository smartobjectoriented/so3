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

#include <heap.h>
#include <memory.h>
#include <crc.h>

#include <avz/memslot.h>
#include <avz/domain.h>
#include <avz/sched.h>
#include <avz/injector.h>
#include <avz/uapi/soo.h>

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
void inject_me(soo_hyp_t *op)
{
	int slotID;
	size_t fdt_size;
	void *fdt_vaddr;
	int dom_size;
	struct domain *domME, *__current;
	addr_t current_pgtable_paddr;
	unsigned long flags;
	void *itb_vaddr;
#ifdef CONFIG_ARCH_ARM64
	uint32_t itb_size;
#endif

#ifdef CONFIG_ARCH_ARM32
	int section_nr;
#endif

	DBG("%s: Preparing ME injection, source image = %lx\n", __func__, op->vaddr);

	flags = local_irq_save();

#ifdef CONFIG_ARCH_ARM64
	/* First, we do a copy of the ME ITB into the avz heap to get independent from Linux mapping (either
	 * in the user space, or in the vmalloc'd area
	 */
	itb_size = *((uint32_t *) op->p_val2);

	itb_vaddr = malloc(itb_size);
	BUG_ON(!itb_vaddr);

	/* op->vaddr: vaddr of itb */
	memcpy(itb_vaddr, (void *) op->vaddr, itb_size);
#else
	itb_vaddr = (void  *) op->vaddr;
#endif

	/* Retrieve the domain size of this ME through its device tree. */
	fit_image_get_data_and_size(itb_vaddr, fit_image_get_node(itb_vaddr, "fdt"), (const void **) &fdt_vaddr, &fdt_size);
	if (!fdt_vaddr) {
		printk("### %s: wrong device tree.\n", __func__);
		BUG();
	}

	dom_size = fdt_getprop_u32_default(fdt_vaddr, "/ME", "domain-size", 0);
	if (dom_size < 0) {
		printk("### %s: wrong domain-size prop/value.\n", __func__);
		BUG();
	}

	/* Find a slotID to store this ME. */
	slotID = get_ME_free_slot(dom_size, ME_state_booting);
	if (slotID == -1)
		goto out;

	domME = domains[slotID];

	/* Set the size of this ME in its own descriptor */
	domME->avz_shared->dom_desc.u.ME.size = memslot[slotID].size;

	/* Now set the pfn base of this ME; this will be useful for the Agency Core subsystem */
	domME->avz_shared->dom_desc.u.ME.pfn = phys_to_pfn(memslot[slotID].base_paddr);

	__current = current_domain;

	mmu_get_current_pgtable(&current_pgtable_paddr);

#ifdef CONFIG_ARCH_ARM32
	/* Get the visibility on the domain image stored in the agency user space area */
	for (section_nr = 0x0; section_nr < 0xc00; section_nr++)
		((uint32_t *) __sys_root_pgtable)[section_nr] = ((uint32_t *) __lva(current_pgtable_paddr & TTBR0_BASE_ADDR_MASK))[section_nr];

	flush_dcache_all();
#endif /* CONFIG_ARCH_ARM32 */

	mmu_switch((void *) idle_domain[smp_processor_id()]->avz_shared->pagetable_paddr);

	/* Clear the RAM allocated to this ME */
	memset((void *) __lva(memslot[slotID].base_paddr), 0, memslot[slotID].size);

	loadME(slotID, itb_vaddr);

	if (construct_ME(domains[slotID]) != 0)
		panic("Could not set up ME guest OS\n");

	/* Switch back to the agency address space */
	set_current_domain(__current);

	mmu_switch((void *) current_pgtable_paddr);

out:
	/* Prepare to return the slotID to the caller. */
	*((unsigned int *) op->p_val1) = slotID;

#ifdef CONFIG_ARCH_ARM64
	free(itb_vaddr);
#endif /* CONFIG_ARCH_ARM64 */

	local_irq_restore(flags);
}
