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
 *
 */

#ifndef FBDEV_GNT_H
#define FBDEV_GNT_H

#include <soo/uapi/soo.h>

/**
 * IPA map for the framebuffer for the given domain, either on actual buffer
 * or the fake one depending on currently shown slot.
 *
 * @param d pointer to the domain to map
 * @param slotID Slot ID of the domain
 */
void fbdev_ipamap_domain(struct domain *d, int slotID);

/**
 * Set the actual framebuffer physical pages ranges for
 * futur mapping.
 *
 * @param fbdev Framebuffer pfns informations.
 */
void fbdev_set_pfns(fbdev_pfns_t *fbdev);

/**
 * Change the currently shown slot ID by remapping corresponding ipa.
 *
 * @param new_slotID to be shown.
 */
void fbdev_change_focus(int new_slotID);

/**
 * Get the frambuffer starting ipa for the current domain.
 */
addr_t fbdev_get_domain_ipa(void);

#endif /* FBDEV_GNT_H */
