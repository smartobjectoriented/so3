/*
 * Copyright (C) 2021 Daniel Rossier <daniel.rossier@soo.tech>
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

#include <list.h>
#include <heap.h>
#include <string.h>

#include <soo/console.h>

#include <avz/uapi/avz.h>

#include <me/common.h>

/**
 * Build an array containing the list of hosts (visited soo)
 *
 * @param hosts_array
 * @return the number of hosts
 */
int concat_hosts(struct list_head *hosts, uint8_t *hosts_array) {
	host_t *host;
	uint8_t *pos = hosts_array;
	int nr = 0;

	list_for_each_entry(host, hosts, list) {

		memcpy(pos, &host->host_entry, sizeof(host_entry_t)-sizeof(void *));
		memcpy(pos+sizeof(host_entry_t)-sizeof(void *), host->host_entry.priv, host->host_entry.priv_len);

		pos += sizeof(host_entry_t)-sizeof(void *)+host->host_entry.priv_len;

		nr++;
	}

	return nr;
}

/**
 * Remove a specific host from our list.
 *
 * @param agencyUID
 */
void del_host(struct list_head *hosts, uint64_t agencyUID) {
	host_t *host;

	list_for_each_entry(host, hosts, list) {
		if (host->host_entry.uid == agencyUID) {
			list_del(&host->list);

			if (host->host_entry.priv_len)
				free(host->host_entry.priv);

			free(host);

			return ;
		}
	}
}

/**
 * Add a new entry in the host list.
 *
 * @param me_common
 * @param agencyUID
 */
void new_host(struct list_head *hosts, uint64_t agencyUID, void *priv, int priv_len) {
	host_t *host;

	host = malloc(sizeof(host_t));
	BUG_ON(!host);

	host->host_entry.uid = agencyUID;

	if (priv_len) {
		host->host_entry.priv = malloc(priv_len);
		BUG_ON(!host->host_entry.priv);

		memcpy(host->host_entry.priv, priv, priv_len);
	}

	host->host_entry.priv_len = priv_len;

	list_add_tail(&host->list, hosts);
}

/**
 * Retrieve the list of host from an array.
 *
 * @param hosts_array
 * @param nr
 */
void expand_hosts(struct list_head *hosts, uint8_t *hosts_array, int nr) {
	int i;
	host_entry_t *host_entry;

	if (!nr)
		return ;

	for (i = 0; i < nr; i++) {

		host_entry = (host_entry_t *) hosts_array;

		new_host(hosts, host_entry->uid, hosts_array+sizeof(host_entry_t)-sizeof(void *), host_entry->priv_len);

		hosts_array += sizeof(host_entry_t)-sizeof(void *)+host_entry->priv_len;
	}
}

/**
 * Reset the list of host
 */
void clear_hosts(struct list_head *hosts) {
	host_t *host, *tmp;

	list_for_each_entry_safe(host, tmp, hosts, list) {

		list_del(&host->list);

		if (host->host_entry.priv_len)
			free(host->host_entry.priv);

		free(host);
	}
}


/**
 * Search for a host corresponding to a specific agencyUID
 *
 * @param agencyUID	UID to compare
 * @return		reference to the host_entry or NULL
 */
host_entry_t *find_host(struct list_head *hosts, uint64_t agencyUID) {
	host_t *host;

	list_for_each_entry(host, hosts, list)
		if (host->host_entry.uid == agencyUID)
			return &host->host_entry;

	return NULL;
}

/**
 * Duplicate a list of hosts
 *
 * @param src The list to be copied
 * @param dst The copied list
 */
void duplicate_hosts(struct list_head *src, struct list_head *dst) {
	host_t *host;

	list_for_each_entry(host, src, list)
		new_host(dst, host->host_entry.uid, host->host_entry.priv, host->host_entry.priv_len);
}

int cmpUID_fn(void *priv, struct list_head *a, struct list_head *b) {
	host_t *host_a, *host_b;

	host_a = list_entry(a, host_t, list);
	host_b = list_entry(b, host_t, list);

	return (host_a->host_entry.uid != host_b->host_entry.uid);
}

/**
 * Sort a list of host by its agencyUID
 *
 * @param hosts
 */
void sort_hosts(struct list_head *hosts) {
	list_sort(NULL, hosts, cmpUID_fn);
}

/**
 * Compare two list of hosts
 *
 * @param incoming_hosts
 * @param visits
 * @return
 */
bool hosts_equals(struct list_head *a, struct list_head *b) {
	LIST_HEAD(tmp_a);
	LIST_HEAD(tmp_b);

	host_t *host_a, *host_b;

	duplicate_hosts(a, &tmp_a);
	duplicate_hosts(b, &tmp_b);

	sort_hosts(&tmp_a);
	sort_hosts(&tmp_b);

	host_b = list_entry(tmp_b.next, host_t, list);
	list_for_each_entry(host_a, &tmp_a, list) {

		if ((&host_b->list == &tmp_b) || (host_a->host_entry.uid != host_b->host_entry.uid)) {
			clear_hosts(&tmp_a);
			clear_hosts(&tmp_b);
			return false;
		}

		host_b = list_entry(host_b->list.next, host_t, list);
	}

	/* The second list must be the same size. */
	if (&host_b->list != &tmp_b) {
		clear_hosts(&tmp_a);
		clear_hosts(&tmp_b);
		return false;
	}

	clear_hosts(&tmp_a);
	clear_hosts(&tmp_b);

	/* Successful */
	return true;
}

/**
 * Merge the "b" list of hosts in the "a" list of hosts.
 * if a host of list "b" is already in list "a", it is simply ignored.
 *
 * @param a
 * @param b
 */
void merge_hosts(struct list_head *a, struct list_head *b) {

	host_t *host_b;

	list_for_each_entry(host_b, b, list) {
		if (!find_host(a, host_b->host_entry.uid))
			new_host(a, host_b->host_entry.uid, host_b->host_entry.priv, host_b->host_entry.priv_len);
	}
}

/**
 * Dump the contents of a list of hosts.
 *
 * @param hosts
 */
void dump_hosts(struct list_head *hosts) {
	host_t *host;

	lprintk("## Dump of host list:\n\n");

	list_for_each_entry(host, hosts, list) {
		lprintk("  * "); lprintk_printlnUID(host->host_entry.uid);
	}

	lprintk("\n--- End of list ---\n");
}

/**
 * Perform a local cooperation in target domain <domID>
 *
 * @param domID
 */
void do_local_cooperation(int domID) {
	do_sync_dom(DOMID_AGENCY, DC_TRIGGER_LOCAL_COOPERATION);
}
