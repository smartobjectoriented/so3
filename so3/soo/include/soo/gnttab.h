/*
 * Copyright (C) 2016-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef GNTTAB_H
#define GNTTAB_H

#include <memory.h>

#include <soo/grant_table.h>

#define NR_GRANT_FRAMES 4
#define NR_GRANT_ENTRIES (NR_GRANT_FRAMES * PAGE_SIZE / sizeof(grant_entry_t))

struct gnttab_free_callback {
	struct gnttab_free_callback *next;
	void (*fn)(void *);
	void *arg;
	u16 count;
};

int gnttab_suspend(void);
int gnttab_resume(void);

int gnttab_grant_foreign_access(domid_t domid, unsigned long frame, int readonly);

/*
 * End access through the given grant reference, iff the grant entry is no
 * longer in use.  Return 1 if the grant entry was freed, 0 if it is still in
 * use.
 */
void gnttab_end_foreign_access_ref(grant_ref_t ref);

/*
 * Eventually end access through the given grant reference, and once that
 * access has been ended, free the given page too.  Access will be ended
 * immediately if the grant entry is not in use, otherwise it will happen
 * some time later.
 */
void gnttab_end_foreign_access(grant_ref_t ref);

int gnttab_grant_foreign_transfer(domid_t domid, unsigned long pfn);

unsigned long gnttab_end_foreign_transfer_ref(grant_ref_t ref);
unsigned long gnttab_end_foreign_transfer(grant_ref_t ref);

int gnttab_query_foreign_access(grant_ref_t ref);

/*
 * operations on reserved batches of grant references
 */
int gnttab_alloc_grant_references(u16 count, grant_ref_t *pprivate_head);

void gnttab_free_grant_reference(grant_ref_t ref);

void gnttab_free_grant_references(grant_ref_t head);

void gnttab_grant_foreign_access_ref(grant_ref_t ref, domid_t domid, unsigned long frame, int readonly);

static inline void gnttab_set_map_op(struct gnttab_map_grant_ref *map, addr_t addr, uint32_t flags, grant_ref_t ref, domid_t domid, unsigned int offset, unsigned int size)
{
	map->host_addr = addr;
	map->flags = flags;
	map->ref = ref;
	map->dom = domid;

	map->handle = 0;

	map->dev_bus_addr = 0;
	map->status = 0;

	map->size = size;   /* By default... */
	map->offset = offset;

}

static inline void gnttab_set_unmap_op(struct gnttab_unmap_grant_ref *unmap, addr_t addr, uint32_t flags, grant_handle_t handle)
{
	unmap->host_addr = addr;
	unmap->dev_bus_addr = 0;
	unmap->flags = flags;
	unmap->ref = 0;

	unmap->handle = handle;

	unmap->dev_bus_addr = 0;
	unmap->status = 0;

	unmap->size = PAGE_SIZE;   /* By default... */
	unmap->offset = 0;

}

void gnttab_init(void);

int grant_table_op(unsigned int cmd, void *uop, unsigned int count);

extern int gnttab_map(struct gnttab_map_grant_ref *op);
extern int gnttab_unmap(struct gnttab_unmap_grant_ref *op);
extern int gnttab_copy(struct gnttab_copy *op);
extern int gnttab_map_with_copy(struct gnttab_map_grant_ref *op);
extern int gnttab_unmap_with_copy(struct gnttab_unmap_grant_ref *op);
extern int gnttab_map_sync_copy(unsigned int handle, unsigned int flags, unsigned int offset, unsigned int size);

int gnttab_unmap_refs(struct gnttab_unmap_grant_ref *unmap_ops, struct gnttab_map_grant_ref *kmap_ops, struct page **pages, unsigned int count);
int gnttab_map_refs(struct gnttab_map_grant_ref *map_ops, struct gnttab_map_grant_ref *kmap_ops, struct page **pages, unsigned int count);

#define gnttab_map_vaddr(map) ((void *)(map.host_virt_addr))

void postmig_gnttab_update(void);


#endif /* GNTTAB_H */
