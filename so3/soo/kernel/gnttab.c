/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2016-2018 Baptiste Delporte <bonel@bonel.net>
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

#include <memory.h>

#include <soo/gnttab.h>
#include <soo/hypervisor.h>

/*
 * Public grant-issuing interface functions
 */

/**
 * @brief Request a grant for a specific pfn to the hypervisor intended to
 * 	  be used by a peer domain.
 * 
 * @param domid  Peer domain which will access the granted page
 * @param pfn    The page to be granted
 * @return 	 The unique reference enabling access to the grant page
 */
int gnttab_grant_foreign_access(domid_t domid, unsigned long pfn)
{
	gnttab_op_t gnttab_op;

	/* Invoke the hypercall to grant access to domid */
	gnttab_op.cmd = GNTTAB_grant_page;
	gnttab_op.domid = domid;
	gnttab_op.pfn = pfn;

	avz_gnttab(&gnttab_op);

	return gnttab_op.ref;
}

/**
 * @brief Query the hypervisor to retrieve a pfn from a grant reference and
 *        do a mapping and return a vaddr
 * 
 * @param domid 
 * @param grant_ref 
 * @param vaddr 
 */
void gnttab_map(domid_t domid, grant_ref_t grant_ref, void **vaddr)
{
	gnttab_op_t gnttab_op;

	gnttab_op.cmd = GNTTAB_map_page;
	gnttab_op.domid = domid;
	gnttab_op.ref = grant_ref;

	avz_gnttab(&gnttab_op);

	*vaddr = (void *) io_map(pfn_to_phys(gnttab_op.pfn), PAGE_SIZE);
	BUG_ON(!*vaddr);
}

void gnttab_end_foreign_access(grant_ref_t ref)
{
	gnttab_op_t gnttab_op;

	gnttab_op.cmd = GNTTAB_revoke_page;
	gnttab_op.ref = ref;

	avz_gnttab(&gnttab_op);
}
