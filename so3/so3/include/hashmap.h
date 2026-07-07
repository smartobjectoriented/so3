/*
 * Copyright (c) 2017-2026 REDS Institute, HEIG-VD
 * Author: Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef HASHMAP_H
#define HASHMAP_H

#include <types.h>

/*
 * A generic hash map associating string keys to void * values.
 *
 * It uses open addressing with linear probing and grows automatically to keep
 * the load factor -- and therefore the lookup cost -- bounded. Keys are copied
 * and owned by the map; values are owned by the caller and left untouched.
 */
struct hashmap;

/*
 * Create a map pre-sized to hold at least `capacity_hint` keys without growing.
 * Returns NULL on allocation failure.
 */
struct hashmap *hashmap_create(size_t capacity_hint);

/* Release the map and its copied keys. Values are not freed. */
void hashmap_free(struct hashmap *map);

/*
 * Associate `value` with `key`, replacing any value previously stored for that
 * key. The key string is copied. Returns 0 on success, -1 on allocation failure.
 */
int hashmap_put(struct hashmap *map, const char *key, void *value);

/* Return the value associated with `key`, or NULL if the key is absent. */
void *hashmap_get(const struct hashmap *map, const char *key);

/* Return the number of entries currently stored in the map. */
size_t hashmap_count(const struct hashmap *map);

#endif /* HASHMAP_H */
