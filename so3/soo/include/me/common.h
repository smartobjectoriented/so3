/*
 * Copyright (C) 2021 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef ME_COMMON_H
#define ME_COMMON_H

#include <avz/uapi/soo.h>

typedef struct {
	uint64_t uid;

	int priv_len;

	/*
	 * MUST REMAIN the last field of this structure (since size computation
	 * and structure organization during concatenation is made with this assumption).
	 */
	void *priv;

} host_entry_t;

typedef struct {

	struct list_head list;

	host_entry_t host_entry;
} host_t;

/*
 * Common structure to help in migration pattern and other.
 * This structure must be allocated within a page (or several contiguous pages).
 */
typedef struct {

	/* Number of visited (host) smart object */
	int soohost_nr;

	/* To store the agencyUID of the smart object in which we are now. */
	uint64_t here;

	/* Another UID to help keeping the origin */
	uint64_t origin;

	/*
	 * List of soo hosts by their agency UID. The first entry
	 * is the smart object origin (set first in pre_activate).
	 * MUST REMAIN the last field of this structure.
	 */
	uint8_t soohosts[0];

} me_common_t;

int concat_hosts(struct list_head *hosts, uint8_t *hosts_array);

/**
 * Remove a specific host from our list.
 *
 * @param agencyUID
 */
void del_host(struct list_head *hosts, uint64_t agencyUID);

/**
 * Add a new entry in the host list.
 *
 * @param me_common
 * @param agencyUID
 */
void new_host(struct list_head *hosts, uint64_t agencyUID, void *priv, int priv_len);

/**
 * Retrieve the list of host from an array.
 *
 * @param hosts_array
 * @param nr
 */
void expand_hosts(struct list_head *hosts, uint8_t *hosts_array, int nr);

/**
 * Reset the list of host
 */
void clear_hosts(struct list_head *hosts);

/**
 * Search for a host corresponding to a specific agencyUID
 *
 * @param agencyUID	UID to compare
 * @return		reference to the host_entry or NULL
 */
host_entry_t *find_host(struct list_head *hosts, uint64_t agencyUID);

/**
 * Duplicate a list of hosts
 *
 * @param src The list to be copied
 * @param dst The copied list
 */
void duplicate_hosts(struct list_head *src, struct list_head *dst);

/**
 * Sort a list of host by its agencyUID
 *
 * @param hosts
 */
void sort_hosts(struct list_head *hosts);

/**
 * Compare two list of hosts
 *
 * @param incoming_hosts
 * @param visits
 * @return
 */
bool hosts_equals(struct list_head *incoming_hosts, struct list_head *);

/**
 * Merge the "b" list of hosts in the "a" list of hosts.
 * if a host of list "b" is already in list "a", it is simply ignored.
 *
 * @param a
 * @param b
 */
void merge_hosts(struct list_head *a, struct list_head *b);

/**
 * Dump the contents of a list of hosts.
 *
 * @param hosts
 */
void dump_hosts(struct list_head *hosts);

/**
 * Perform a local cooperation in target domain <domID>
 *
 * @param domID
 */
void do_local_cooperation(int domID);

#endif /* ME_COMMON_H */
