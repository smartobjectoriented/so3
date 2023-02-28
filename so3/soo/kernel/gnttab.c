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

#include <list.h>
#include <heap.h>
#include <memory.h>

#include <asm/cacheflush.h>			/* cache flushing ops */
#include <asm/mmu.h>

#include <device/irq.h>

#include <soo/vbus.h>
#include <soo/gnttab.h>
#include <soo/debug.h>
#include <soo/hypervisor.h>
#include <soo/vbstore.h>
#include <soo/console.h>
#include <soo/errno.h>

static grant_entry_t *gnttab;

/* External tools reserve first few grant table entries. -> TO BE REMOVED IN A NEAR FUTURE ! */
#define NR_RESERVED_ENTRIES 	8
#define GNTTAB_LIST_END 	(NR_GRANT_ENTRIES + 1)

static grant_ref_t gnttab_list[NR_GRANT_ENTRIES];
static int gnttab_free_count;
static grant_ref_t gnttab_free_head;
static DEFINE_SPINLOCK(gnttab_list_lock);

static inline int get_order_from_pages(unsigned long nr_pages)
{
	int order;
	nr_pages--;
	for (order = 0; nr_pages; order++)
		nr_pages >>= 1;

	return order;
}

struct handle_grant {
	grant_ref_t gref;
	unsigned int domid;
	unsigned int handle;
	uint64_t host_addr;

	/* Sub-page mapping...*/
	unsigned int offset;
	unsigned int size;

	struct list_head list;
};

/*
 * Number of associations pfn<->frame table entry
 * Basically, number of foreign pages + number of pages necessary for the foreign grant table
 */
#define NR_FRAME_TABLE_ENTRIES	(NR_GRANT_FRAMES + 2)

static inline u32 atomic_cmpxchg_u16(volatile u16 *v, u16 old, u16 new)
{
	u16 ret;
	unsigned long flags;

	flags = local_irq_save();

	ret = *v;
	if (likely(ret == old))
		*v = new;

	local_irq_restore(flags);

	return ret;
}

static int get_free_entries(int count)
{
	unsigned long flags;
	int ref;
	grant_ref_t head;

	flags = spin_lock_irqsave(&gnttab_list_lock);

	if (gnttab_free_count < count) {
		spin_unlock_irqrestore(&gnttab_list_lock, flags);
		return -1;
	}

	ref = head = gnttab_free_head;
	gnttab_free_count -= count;

	while (count-- > 1)
		head = gnttab_list[head];

	gnttab_free_head = gnttab_list[head];
	gnttab_list[head] = GNTTAB_LIST_END;

	spin_unlock_irqrestore(&gnttab_list_lock, flags);
	return ref;
}

#define get_free_entry() get_free_entries(1)

static void put_free_entry(grant_ref_t ref)
{
	unsigned long flags;

	flags = spin_lock_irqsave(&gnttab_list_lock);

	gnttab_list[ref] = gnttab_free_head;
	gnttab_free_head = ref;
	gnttab_free_count++;

	spin_unlock_irqrestore(&gnttab_list_lock, flags);
}

/*
 * Public grant-issuing interface functions
 */

int gnttab_grant_foreign_access(domid_t domid, unsigned long frame, int readonly)
{
	int ref;

	if (unlikely((ref = get_free_entry()) == -1))
		BUG();

	gnttab[ref].frame = frame;
	gnttab[ref].domid = domid;
	wmb();
	gnttab[ref].flags = GTF_permit_access | (readonly ? GTF_readonly : 0);

	DBG("%s(%d, %08x), ref=%u\n", __func__, domid, frame, ref);

	return ref;
}

void gnttab_grant_foreign_access_ref(grant_ref_t ref, domid_t domid, unsigned long frame, int readonly)
{
	gnttab[ref].frame = frame;
	gnttab[ref].domid = domid;
	wmb();
	gnttab[ref].flags = GTF_permit_access | (readonly ? GTF_readonly : 0);

}

int gnttab_query_foreign_access(grant_ref_t ref)
{
	u16 nflags;

	nflags = gnttab[ref].flags;

	return (nflags & (GTF_reading|GTF_writing));
}

void gnttab_end_foreign_access_ref(grant_ref_t ref)
{
	u16 flags, nflags;

	nflags = gnttab[ref].flags;
	do {
		if ((flags = nflags) & (GTF_reading|GTF_writing)) {
			printk("WARNING: g.e. still in use!\n");
			BUG();
		}
	}
	while ((nflags = atomic_cmpxchg_u16(&gnttab[ref].flags, flags, 0)) != flags) ;
}

void gnttab_end_foreign_access(grant_ref_t ref)
{
	gnttab_end_foreign_access_ref(ref);
	put_free_entry(ref);

}

void gnttab_free_grant_reference(grant_ref_t ref)
{
	put_free_entry(ref);
}

void gnttab_free_grant_references(grant_ref_t head)
{
	grant_ref_t ref;
	unsigned long flags;
	int count = 1;
	if (head == GNTTAB_LIST_END)
		return;
	flags = spin_lock_irqsave(&gnttab_list_lock);
	ref = head;
	while (gnttab_list[ref] != GNTTAB_LIST_END) {
		ref = gnttab_list[ref];
		count++;
	}
	gnttab_list[ref] = gnttab_free_head;
	gnttab_free_head = head;
	gnttab_free_count += count;

	spin_unlock_irqrestore(&gnttab_list_lock, flags);
}

int gnttab_alloc_grant_references(u16 count, grant_ref_t *head)
{
	int h = get_free_entries(count);

	if (h == -1)
		return -ENOSPC;

	*head = h;

	return 0;
}

/*
 * Remove the grant table and all foreign entries.
 */
int gnttab_remove(bool with_vbus) {
	char path[20];

	DBG("Removing grant table ...\n");

	/* Free previous entries */
	free_contig_vpages((addr_t) gnttab, NR_GRANT_FRAMES);

	if (with_vbus) {
		sprintf(path, "domain/gnttab/%i", ME_domID());
		vbus_rm(VBT_NIL, path, "pfn");
	}

	return 0;
}

/*
 * Grant table initialization function.
 */
void gnttab_init(void)
{
	struct vbus_transaction vbt;
	char buf[VBS_KEY_LENGTH], path[VBS_KEY_LENGTH];
	int i;

	DBG("%s: setting up...\n", __func__);

	/* First allocate the current domain grant table. */
	gnttab = (void *) get_contig_free_vpages(NR_GRANT_FRAMES);

	if (gnttab == NULL)
	{
		printk("%s/%d: Failed to alloc grant table\n", __FILE__, __LINE__);
		BUG();
	}

	DBG("Exporting grant_ref table pfn %05lx virt %p \n", phys_to_pfn(virt_to_phys_pt((addr_t) gnttab)), gnttab);

	for (i = NR_RESERVED_ENTRIES; i < NR_GRANT_ENTRIES; i++)
		gnttab_list[i] = i + 1;

	gnttab_free_count = NR_GRANT_ENTRIES - NR_RESERVED_ENTRIES;
	gnttab_free_head  = NR_RESERVED_ENTRIES;

	/* Write our shared page address of our grant table into vbstore */
	DBG0("Writing our grant table pfn to vbstore ...\n");

	vbus_transaction_start(&vbt);

	sprintf(buf, "%lX", phys_to_pfn(virt_to_phys_pt((addr_t) gnttab)));
	sprintf(path, "domain/gnttab/%i", ME_domID());

	vbus_write(vbt, path, "pfn", buf);

	vbus_transaction_end(vbt);

	DBG0("End of gnttab_init. Well done!\n");
}

/*
 * Update the grant table for the post-migrated domain.
 * Update the watches as well.
 */
void postmig_gnttab_update(void) {

	gnttab_remove(false);

	/* At the moment, perform a full rebuild of grant table */
	gnttab_init();

}

