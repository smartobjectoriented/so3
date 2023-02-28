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

#ifndef __GRANT_TABLE_H__
#define __GRANT_TABLE_H__

#include <types.h>

#include <soo/avz.h>

#define GRANT_INVALID_REF	0

struct grant_entry {
	/* GTF_xxx: various type and flag information.   */
	uint16_t flags;
	/* The domain being granted foreign privileges.  */
	domid_t  domid;
	/*
	 * GTF_permit_access: Frame that @domid is allowed to map and access.
	 * GTF_accept_transfer: Frame whose ownership transferred by @domid.
	 */
	uint32_t frame;
};
typedef struct grant_entry grant_entry_t;

/*
 * Type of grant entry.
 *  GTF_invalid: This grant entry grants no privileges.

 */
#define GTF_invalid         (0U<<0)
#define GTF_permit_access   (1U<<0)

/*
 * Subflags for GTF_permit_access.
 *  GTF_readonly: Restrict @domid to read-only mappings and accesses. [GST]
 *  GTF_reading: Grant entry is currently mapped for reading by @domid.
 *  GTF_writing: Grant entry is currently mapped for writing by @domid.
 */
#define _GTF_readonly       (2)
#define GTF_readonly        (1U<<_GTF_readonly)
#define _GTF_reading        (3)
#define GTF_reading         (1U<<_GTF_reading)
#define _GTF_writing        (4)
#define GTF_writing         (1U<<_GTF_writing)




/***********************************
 * GRANT TABLE QUERIES AND USES
 */

/*
 * Reference to a grant entry in a specified domain's grant table.
 */
typedef uint32_t grant_ref_t;

/*
 * Handle to track a mapping created via a grant reference.
 */
typedef uint32_t grant_handle_t;

/*
 * GNTTABOP_map_grant_ref: Map the grant entry (<dom>,<ref>) for access
 * by devices and/or host CPUs. If successful, <handle> is a tracking number
 * that must be presented later to destroy the mapping(s). On error, <handle>
 * is a negative status code.
 * NOTES:
 *  1. If GNTMAP_device_map is specified then <dev_bus_addr> is the address
 *     via which I/O devices may access the granted frame.
 *  2. If GNTMAP_host_map is specified then a mapping will be added at
 *     either a host virtual address in the current address space, or at
 *     a PTE at the specified machine address.  The type of mapping to
 *     perform is selected through the GNTMAP_contains_pte flag, and the
 *     address is specified in <host_addr>.
 *  3. Mappings should only be destroyed via GNTTABOP_unmap_grant_ref. If a
 *     host mapping is destroyed by other means then it is *NOT* guaranteed
 *     to be accounted to the correct grant reference!
 */
#define GNTTABOP_map_grant_ref        0
struct gnttab_map_grant_ref {
	/* IN parameters. */
	uint64_t host_addr;
	uint32_t flags;               /* GNTMAP_* */
	grant_ref_t ref;
	domid_t  dom;
	/* OUT parameters. */
	int16_t  status;              /* GNTST_* */
	grant_handle_t handle;
	uint64_t dev_bus_addr;

	uint32_t offset;
	uint32_t size;
};
typedef struct gnttab_map_grant_ref gnttab_map_grant_ref_t;

/*
 * GNTTABOP_unmap_grant_ref: Destroy one or more grant-reference mappings
 * tracked by <handle>. If <host_addr> or <dev_bus_addr> is zero, that
 * field is ignored. If non-zero, they must refer to a device/host mapping
 * that is tracked by <handle>
 * NOTES:
 *  1. The call may fail in an undefined manner if either mapping is not
 *     tracked by <handle>.
 *  3. After executing a batch of unmaps, it is guaranteed that no stale
 *     mappings will remain in the device or host TLBs.
 */
#define GNTTABOP_unmap_grant_ref      1
struct gnttab_unmap_grant_ref {
	/* IN parameters. */
	uint32_t flags;
	grant_ref_t ref;
	domid_t  dom;
	uint64_t host_addr;
	uint64_t dev_bus_addr;
	grant_handle_t handle;
	/* OUT parameters. */
	int16_t  status;              /* GNTST_* */

	uint32_t offset;
	uint32_t size;
};
typedef struct gnttab_unmap_grant_ref gnttab_unmap_grant_ref_t;

/*
 * GNTTABOP_copy: Hypervisor based copy
 * source and destinations can be eithers MFNs or, for foreign domains,
 * grant references. the foreign domain has to grant read/write access
 * in its grant table.
 *
 * The flags specify what type source and destinations are (either MFN
 * or grant reference).
 *
 * Note that this can also be used to copy data between two domains
 * via a third party if the source and destination domains had previously
 * grant appropriate access to their pages to the third party.
 *
 * source_offset specifies an offset in the source frame, dest_offset
 * the offset in the target frame and  len specifies the number of
 * bytes to be copied.
 */

#define _GNTCOPY_source_gref      (0)
#define GNTCOPY_source_gref       (1 << _GNTCOPY_source_gref)
#define _GNTCOPY_dest_gref        (1)
#define GNTCOPY_dest_gref         (1 << _GNTCOPY_dest_gref)
#define _GNTCOPY_sync_ref    		  (2)
#define GNTCOPY_sync_ref     			(1 << _GNTCOPY_sync_ref)

#define GNTTABOP_copy                 5
typedef struct gnttab_copy {
	/* IN parameters. */
	struct {
		union {
			grant_ref_t ref;
			unsigned int gmfn;
		} u;
		domid_t  domid;
		uint16_t offset;
	} source, dest;
	uint16_t      len;
	uint16_t      flags;          /* GNTCOPY_* */
	/* OUT parameters. */
	int16_t       status;
	unsigned int handle;
} gnttab_copy_t;


/* Map the grant entry for access by host CPUs. */
#define _GNTMAP_host_map        (1)
#define GNTMAP_host_map         (1<<_GNTMAP_host_map)
/* Accesses to the granted frame will be restricted to read-only access. */
#define _GNTMAP_readonly        (2)
#define GNTMAP_readonly         (1<<_GNTMAP_readonly)

#define _GNTMAP_with_copy       (5)
#define GNTMAP_with_copy        (1 << _GNTMAP_with_copy)


/*
 * Values for error status returns. All errors are -ve.
 */
#define GNTST_okay             (0)  /* Normal return.                        */
#define GNTST_general_error    (-1) /* General undefined error.              */
#define GNTST_bad_domain       (-2) /* Unrecognsed domain id.                */
#define GNTST_bad_gntref       (-3) /* Unrecognised or inappropriate gntref. */
#define GNTST_bad_handle       (-4) /* Unrecognised or inappropriate handle. */
#define GNTST_bad_virt_addr    (-5) /* Inappropriate virtual address to map. */
#define GNTST_bad_dev_addr     (-6) /* Inappropriate device address to unmap.*/
#define GNTST_no_device_space  (-7) /* Out of space in I/O MMU.              */
#define GNTST_permission_denied (-8) /* Not enough privilege for operation.  */
#define GNTST_bad_page         (-9) /* Specified page was invalid for op.    */
#define GNTST_bad_copy_arg    (-10) /* copy arguments cross page boundary.   */
#define GNTST_address_too_big (-11) /* transfer page address too large.      */
#define GNTST_eagain          (-12) /* Operation not done; try again.        */

#define GNTTABOP_error_msgs {                   \
		"okay",                                     \
		"undefined error",                          \
		"unrecognised domain id",                   \
		"invalid grant reference",                  \
		"invalid mapping handle",                   \
		"invalid virtual address",                  \
		"invalid device address",                   \
		"no spare translation slot in the I/O MMU", \
		"permission denied",                        \
		"bad page",                                 \
		"copy arguments cross page boundary",       \
		"page address size too large"               \
}

#endif /* __GRANT_TABLE_H__ */

