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

#include <soo/uapi/soo.h>

int gnttab_suspend(void);
int gnttab_resume(void);

int gnttab_grant_foreign_access(domid_t domid, unsigned long frame);

/*
 * Eventually end access through the given grant reference, and once that
 * access has been ended, free the given page too.  Access will be ended
 * immediately if the grant entry is not in use, otherwise it will happen
 * some time later.
 */
void gnttab_end_foreign_access(grant_ref_t ref);

void gnttab_map(domid_t domid, grant_ref_t grant_ref, void **vaddr);

void postmig_gnttab_update(void);

#endif /* GNTTAB_H */
