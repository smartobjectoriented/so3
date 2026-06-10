/*
 * Copyright (C) 2024-2025 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <spinlock.h>
#include <heap.h>

#include <asm/processor.h>

#include <avz/domain.h>
#include <avz/sched.h>

#include <avz/gnttab.h>

static DEFINE_SPINLOCK(gnttab_lock);

void gnttab_init(struct domain *d)
{
	INIT_LIST_HEAD(&d->gnttab);
}

/**
 * @brief Get the highest value of ref of a grant table
 * 
 * @param gnttab 
 * @return grant_ref_t : The highest reference
 */
grant_ref_t get_ref_max(struct list_head *gnttab)
{
	gnttab_t *cur;
	grant_ref_t ref = GRANT_INVALID_REF;

	list_for_each_entry(cur, gnttab, list)
		ref = ((cur->ref > ref) ? cur->ref : ref);

	ref = ((ref == GRANT_INVALID_REF) ? 1 : ref);

	return ref;
}

/**
 * @brief Find a granted page from a specific domain
 * 
 * @param ref reference to the granted page
 * @param origin_domid granted by this domain (ref is not unique accross granting domains)
 * @return gnttab_t* 
 */
gnttab_t *pick_granted_entry(grant_ref_t ref, domid_t origin_domid)
{
	gnttab_t *cur;

	list_for_each_entry(cur, &domains[origin_domid]->gnttab, list) {
		if ((cur->ref == ref) && (cur->origin_domid == origin_domid) &&
		    (cur->target_domid == current_domain->avz_shared->domID))
			return cur; /* Found! */
	}

	return NULL;
}

/**
 * @brief Create a new grant table entry in the gnttab of a domain
 * 
 * @param d The domain containing the grant table
 * @param target_domid The domain concerned by this grant
 * @param pfn The real frame number of the page to be granted
 */
gnttab_t *new_gnttab_entry(struct domain *d, domid_t target_domid, addr_t pfn)
{
	gnttab_t *gnttab;

	gnttab = malloc(sizeof(gnttab_t));
	BUG_ON(!gnttab);

	gnttab->origin_domid = d->avz_shared->domID;
	gnttab->target_domid = target_domid;
	gnttab->pfn = pfn;

	/* Determine the ref number for this entry */
	gnttab->ref = get_ref_max(&d->gnttab) + 1;

	list_add_tail(&gnttab->list, &d->gnttab);

	return gnttab;
}

void revoke_gnttab_entry(struct domain *d, grant_ref_t ref)
{
	gnttab_t *cur;

	list_for_each_entry(cur, &d->gnttab, list) {
		if (cur->ref == ref) {
			list_del(&cur->list);
			free(cur);
			return;
		}
	}

	/* In case of resuming, the list of gnttab is empty. */
}

/**
 * @brief Find a free grant pfn 
 * 
 * @param d 
 * @return addr_t pfn free to be used for mapping to granted page
 */
addr_t allocate_grant_pfn(struct domain *d)
{
	int i;

	for (i = 0; i < NR_GRANT_PFN; i++)
		if (d->grant_pfn[i].free) {
			d->grant_pfn[i].free = false;
			return d->grant_pfn[i].pfn;
		}

	/* Now, we don't consider no free grant pfn */
	BUG();

	return 0; /* Make gcc happy :-) */
}

/**
 * @brief Map the grant page associated to vbstore in the IPA domain of the ME
 *
 * @param target_domid the ME which needs the vbstore pfn
 * @param pfn 
 * @return 
 */
addr_t map_vbstore_pfn(int target_domid, int pfn)
{
	gnttab_t *cur;
	addr_t grant_paddr;
	struct domain *d;

	d = domains[target_domid];

	list_for_each_entry(cur, &agency->gnttab, list) {
		if (cur->target_domid == target_domid) {
			/* Here, we get an IPA address corresponding to the grant page */
			if (pfn == 0)
				grant_paddr = pfn_to_phys(allocate_grant_pfn(d));
			else
				grant_paddr = pfn_to_phys(pfn);

			__create_mapping((addr_t *) d->pagetable_vaddr, grant_paddr, pfn_to_phys(cur->pfn), PAGE_SIZE, true,
					 S2);

			return phys_to_pfn(grant_paddr);
		}
	}

	BUG();

	return 0;
}

/**
 * @brief Hypercall entry for grant table related operations.
 * 
 * @param args gnttab detail structure
 */
void do_gnttab(gnttab_op_t *args)
{
	struct domain *d;
	addr_t paddr, grant_paddr;
	gnttab_t *gnttab;

	spin_lock(&gnttab_lock);

	d = current_domain;

	switch (args->cmd) {
	case GNTTAB_grant_page:

		/* Create a new entry in the list of gnttab page */

		paddr = ipa_to_pa(DOM_TO_MEMSLOT(d->avz_shared->domID), pfn_to_phys(args->pfn));

		gnttab = new_gnttab_entry(d, args->domid, phys_to_pfn(paddr));

		args->ref = gnttab->ref;

		break;

	case GNTTAB_revoke_page:
		revoke_gnttab_entry(d, args->ref);
		break;

	case GNTTAB_map_page:

		/* Retrieve the original granted page */
		gnttab = pick_granted_entry(args->ref, args->domid);
		BUG_ON(!gnttab);

		/* Here, we get an IPA address corresponding to the grant page */
		grant_paddr = pfn_to_phys(allocate_grant_pfn(d));

		/* This pfn will be exported to the domain */
		args->pfn = phys_to_pfn(grant_paddr);

		__create_mapping((addr_t *) d->pagetable_vaddr, grant_paddr, pfn_to_phys(gnttab->pfn), PAGE_SIZE, true, S2);

		break;

	case GNTTAB_unmap_page:
		/* to be completed */
		break;
	}

	spin_unlock(&gnttab_lock);
}
