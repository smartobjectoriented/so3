/*
 * Copyright (C) 2026 Clément Dieperink <clement.dieperink@heig-vd.ch>
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
 */

#include <heap.h>
#include <avz/fbdev_gnt.h>
#include <avz/domain.h>
#include <avz/memslot.h>
#include <avz/sched.h>

typedef struct {
	fbdev_pfns_t fbdev_pfns;
	void *fake_fbdev;
	int current_slotID;
} fbdev_priv_t;

static fbdev_priv_t priv = {};

static void __map_fbdev(struct domain *d, const fbdev_pfns_t *pfn_info)
{
	size_t i;
	addr_t phys_addr;
	addr_t ipa_addr;
	size_t size;
	void *pgtable;

	pgtable = (void *) d->pagetable_l0_vaddr;
	ipa_addr = pfn_to_phys(d->fbdev_start_pfn);

	/* Map all distincts ranges of the framebuffer to the capsule */
	for (i = 0; i < pfn_info->pfn_count; i++) {
		phys_addr = pfn_to_phys(pfn_info->pfn[i]);
		size = pfn_info->page_count[i] * PAGE_SIZE;

		__create_mapping(pgtable, ipa_addr, phys_addr, size, true, S2);

		ipa_addr += size;
	}
}

static void __map_fake_fbdev(struct domain *d, const fbdev_pfns_t *real_fb)
{
	size_t i, j;
	addr_t phys_addr;
	addr_t ipa_addr;
	void *pgtable;

	/* One time malloc of fake framebuffer page */
	if (priv.fake_fbdev == NULL) {
		priv.fake_fbdev = malloc(PAGE_SIZE);
		BUG_ON(!priv.fake_fbdev);
	}

	pgtable = (void *) d->pagetable_l0_vaddr;
	ipa_addr = pfn_to_phys(d->fbdev_start_pfn);
	phys_addr = __pa(priv.fake_fbdev);

	/* Map the capsule framebuffer to the fake one */
	for (i = 0; i < real_fb->pfn_count; i++) {
		for (j = 0; j < real_fb->page_count[i]; j++) {
			__create_mapping(pgtable, ipa_addr, phys_addr, PAGE_SIZE, true, S2);

			ipa_addr += PAGE_SIZE;
		}
	}
}

void fbdev_ipamap_domain(struct domain *d, int slotID)
{
	/* Only capsules have virtual framebuffer */
	if ((slotID < MEMSLOT_BASE) && !memslot[slotID].busy)
		return;

	if (slotID == priv.current_slotID)
		__map_fbdev(d, &priv.fbdev_pfns);
	else
		__map_fake_fbdev(d, &priv.fbdev_pfns);
}

void fbdev_set_pfns(fbdev_pfns_t *fbdev)
{
	int slotID;

	memcpy(&priv.fbdev_pfns, fbdev, sizeof(*fbdev));

	/* Map framebuffer to all capsules. */
	for (slotID = MEMSLOT_BASE; slotID < MEMSLOT_NR; slotID++)
		if (memslot[slotID].busy)
			fbdev_ipamap_domain(domains[slotID], slotID);
}

void fbdev_change_focus(int new_slotID)
{
	/* Remap old capsule to fake framebuffer */
	if ((priv.current_slotID >= MEMSLOT_BASE) && memslot[priv.current_slotID].busy)
		__map_fake_fbdev(domains[priv.current_slotID], &priv.fbdev_pfns);

	/* Map the new capsule to the framebuffer */
	if ((new_slotID >= MEMSLOT_BASE) && memslot[new_slotID].busy)
		__map_fbdev(domains[new_slotID], &priv.fbdev_pfns);

	priv.current_slotID = new_slotID;
}

addr_t fbdev_get_domain_ipa(void)
{
	return pfn_to_phys(current_domain->fbdev_start_pfn);
}
