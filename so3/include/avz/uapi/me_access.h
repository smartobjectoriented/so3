/*
 * Copyright (C) 2016-2021 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef ME_ACCESS_H
#define ME_ACCESS_H

#ifndef __KERNEL__

#include <stdbool.h>
#include <stdint.h>

#else

#include <common.h>

#ifdef CONFIG_AVZ
#include <types.h>
#endif

#endif

#ifndef CONFIG_AVZ

#ifndef __SO3__
#include <linux/types.h>
typedef uint64_t addr_t;
#endif

#endif

#include <types.h>

/*
 * Capabilities for the Species Aptitude Descriptor (SPAD) structure
 *
 */

#define SPADCAP_HEATING_CONTROL		(1 << 0)

/*
 * Species Aptitude Descriptor (SPAD)
 */
typedef struct {

	/* Indicate if the ME accepts to collaborate with other ME */
	bool valid;

	/* SPAD capabilities */
	uint64_t spadcaps;

} spad_t;

/* This structure is used as the first field of the ME buffer frame header */
typedef struct {

	uint32_t ME_size;
	uint32_t size_mig_structure;

} ME_info_transfer_t;

/*
 * ME states:
 * - ME_state_booting:		ME is currently booting...
 * - ME_state_preparing:	ME is being paused during the boot process, in the case of an injection, before the frontend initialization
 * - ME_state_living:		ME is full-functional and activated (all frontend devices are consistent)
 * - ME_state_suspended:	ME is suspended before migrating. This state is maintained for the resident ME instance
 * - ME_state_migrating:	ME just arrived in SOO
 * - ME_state_dormant:		ME is resident, but not living (running) - all frontends are closed/shutdown
 * - ME_state_killed:		ME has been killed before to be resumed
 * - ME_state_terminated:	ME has been terminated (by a force_terminate)
 * - ME_state_dead:		ME does not exist
 */
typedef enum {
	ME_state_booting,
	ME_state_preparing,
	ME_state_living,
	ME_state_suspended,
	ME_state_migrating,
	ME_state_dormant,
	ME_state_killed,
	ME_state_terminated,
	ME_state_dead
} ME_state_t;

/* Keep information about slot availability
 * FREE:	the slot is available (no ME)
 * BUSY:	the slot is allocated a ME
 */
typedef enum {
	ME_SLOT_FREE,
	ME_SLOT_BUSY
} ME_slotState_t;

/*
 * ME descriptor
 *
 * WARNING !! Be careful when modifying this structure. It *MUST* be aligned with
 * the same structure used in the ME.
 */
typedef struct {
	unsigned int	slotID;

	ME_state_t	state;

	unsigned int	size; /* Size of the ME */
	unsigned int	pfn;

	uint64_t	spid; /* Species ID */
	spad_t		spad; /* ME Species Aptitude Descriptor */
} ME_desc_t;

/* ME ID related information */
#define ME_NAME_SIZE				40
#define ME_SHORTDESC_SIZE			1024

/*
 * Definition of ME ID information used by functions which need
 * to get a list of running MEs with their information.
 */
typedef struct {
	uint32_t slotID;
	ME_state_t state;

	uint64_t spid;
	uint64_t spadcaps;

	char name[ME_NAME_SIZE];
	char shortdesc[ME_SHORTDESC_SIZE];
} ME_id_t;

/* Fixed size for the header of the ME buffer frame (max.) */
#define ME_EXTRA_BUFFER_SIZE (1024 * 1024)

#ifndef CONFIG_AVZ

int get_ME_state(void);
void set_ME_state(ME_state_t state);

int32_t get_ME_free_slot(uint32_t size);

bool get_ME_id(uint32_t slotID, ME_id_t *ME_id);

void get_ME_id_array(ME_id_t *ME_id_array);
char *xml_prepare_id_array(ME_id_t *ME_id_array);

ME_desc_t *get_ME_desc(void);

#endif /* !CONFIG_AVZ */

#endif /* ME_ACCESS_H */


