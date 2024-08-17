/*
 * Copyright (C) 2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <string.h>
#include <heap.h>
#include <console.h>
#include <common.h>
#include <limits.h>

#include <avz/sched.h>
#include <avz/keyhandler.h>
#include <avz/logbool.h>

typedef entry_t * entryp_t;

logbool_hashtable_t *avz_logbool_ht;

/* Create a new hashtable. */
void *ht_create(int size) {

	logbool_hashtable_t *hashtable = NULL;
	int i;

	/* Allocate the table itself. */
	hashtable = malloc(sizeof(logbool_hashtable_t));
	BUG_ON(!hashtable);

	/* Allocate pointers to the head nodes. */
	hashtable->table = malloc(size * sizeof(entryp_t));
	BUG_ON(!hashtable->table);

	for (i = 0; i < size; i++)
		hashtable->table[i] = NULL;

	hashtable->size = size;

	return hashtable;
}

/* Hash a string for a particular hash table. */
int ht_hash(logbool_hashtable_t *hashtable, char *key) {

	unsigned long int hashval = 0;
	int i = 0;

	/* Convert our string to an integer */
	while ((hashval < ULONG_MAX) && (i < strlen(key))) {
		hashval = hashval << 8;
		hashval += key[i];
		i++;
	}

	return hashval % hashtable->size;
}

/* Create a key-value pair. */
entry_t *ht_newpair(char *key, logbool_t value) {
	entry_t *newpair;

	if ((newpair = malloc(sizeof(entry_t))) == NULL)
		return NULL;

	if ((newpair->key = strdup(key)) == NULL)
		return NULL;

	value.count = 0;

	newpair->value = value;
	newpair->next = NULL;

	return newpair;
}

/* Insert a key-value pair into a hash table. */
void ht_set(logbool_hashtable_t *hashtable, char *key, logbool_t value) {
	int bin = 0;
	entry_t *newpair = NULL;
	entry_t *next = NULL;
	entry_t *last = NULL;

	bin = ht_hash(hashtable, key);

	next = hashtable->table[bin];

	while ((next != NULL) && (next->key != NULL) && strcmp(key, next->key) > 0) {
		last = next;
		next = next->next;
	}

	/* There's already a pair.  Let's replace that string. */
	if ((next != NULL) && (next->key != NULL) && strcmp(key, next->key) == 0)
		next->value = value;

	/* Nope, could't find it.  Time to grow a pair. */
	else {

		newpair = ht_newpair(key, value);

		/* We're at the start of the linked list in this bin. */
		if (next == hashtable->table[bin]) {
			newpair->next = next;
			hashtable->table[bin] = newpair;

		/* We're at the end of the linked list in this bin. */
		} else if (next == NULL) {
			last->next = newpair;

		/* We're in the middle of the list. */
		} else  {
			newpair->next = next;
			last->next = newpair;
		}
	}
}

/* Retrieve a key-value pair from a hash table. */
logbool_t *ht_get(logbool_hashtable_t *hashtable, char *key) {
	int bin = 0;
	entry_t *pair;

	bin = ht_hash(hashtable, key);

	/* Step through the bin, looking for our value. */
	pair = hashtable->table[bin];
	while ((pair != NULL) && (pair->key != NULL) && strcmp(key, pair->key) > 0)
		pair = pair->next;

	/* Did we actually find anything? */
	if ((pair == NULL) || (pair->key == NULL) || (strcmp(key, pair->key) != 0))
		return NULL;
	else
		return &pair->value;

}

void ht_destroy(logbool_hashtable_t *hashtable) {
	BUG_ON(!hashtable->table);

	free(hashtable->table);
	free(hashtable);
}

void dump_logbool(logbool_hashtable_t *ht) {
    int i;

    for (i = 0; i < ht->size; i++)
        if (ht->table[i] != NULL)
            printk("     %d: %s: on CPU: %d state: %d - # visits: %d\n", i, ht->table[i]->key, ht->table[i]->value.cpu, ht->table[i]->value.section_state,
              ht->table[i]->value.count);
}

void dump_all_logbool(unsigned char key) {
	int i;

	printk("***** Dumping logbool of AVZ *****\n");
	dump_logbool(avz_logbool_ht);

	/* Dump logbool hashtable of all domains including avz */

	printk("***** Dumping non-RT Agency *****\n");
	dump_logbool(domains[DOMID_AGENCY]->avz_shared->logbool_ht);

	printk("***** Dumping RT Agency *****\n");
	dump_logbool(domains[DOMID_AGENCY_RT]->avz_shared->logbool_ht);

	printk("***** Dumping ME logbool activities *****\n");
	for (i = 1; i < MAX_DOMAINS; i++) {
		if (domains[i] != NULL) {
			printk("***** Dumping logbool of domain %d *****\n", i);
			dump_logbool(domains[i]->avz_shared->logbool_ht);
		}
	}
}

struct keyhandler dump_logbool_keyhandler = {
		.fn = dump_all_logbool,
		.desc = "dump logbool hashtable"
};


void logbool_init(void) {

	/* Create a logbool hashtable for AVZ */
	avz_logbool_ht = ht_create(LOGBOOL_HT_SIZE);

	register_keyhandler('x', &dump_logbool_keyhandler);
}
