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

#include <heap.h>
#include <math.h>
#include <string.h>
#include <types.h>

#include <hashmap.h>

/* Smallest table size; must be a power of two. */
#define HASHMAP_MIN_CAPACITY 8u

/*
 * One table slot. An empty slot has a NULL key. There is no removal operation,
 * so no tombstones are needed and a probe simply stops at the first empty slot.
 */
struct hashmap_slot {
	char *key; /* owned copy of the key, NULL when the slot is empty */
	void *value;
};

struct hashmap {
	struct hashmap_slot *slots;
	size_t capacity; /* always a power of two */
	size_t count; /* number of live entries */
};

/* FNV-1a hash over a NUL-terminated string. */
static unsigned int str_hash(const char *s)
{
	unsigned int h = 2166136261u;

	while (*s) {
		h ^= (unsigned char) *s++;
		h *= 16777619u;
	}

	return h;
}

struct hashmap *hashmap_create(size_t capacity_hint)
{
	struct hashmap *map = (struct hashmap *) malloc(sizeof(*map));

	if (!map)
		return NULL;

	/* Size the table so `capacity_hint` keys stay below a 0.5 load factor. */
	map->capacity = round_up_pow2(capacity_hint * 2);
	if (map->capacity < HASHMAP_MIN_CAPACITY)
		map->capacity = HASHMAP_MIN_CAPACITY;
	map->count = 0;
	map->slots = (struct hashmap_slot *) calloc(map->capacity, sizeof(*map->slots));

	if (!map->slots) {
		free(map);
		return NULL;
	}

	return map;
}

void hashmap_free(struct hashmap *map)
{
	size_t i;

	if (!map)
		return;

	for (i = 0; i < map->capacity; i++)
		if (map->slots[i].key)
			free(map->slots[i].key);

	free(map->slots);
	free(map);
}

/* Insert an already-owned key/value pair; assumes a free slot exists and the
 * key is not already present (used for fresh insertions and rehashing). */
static void hashmap_place(struct hashmap_slot *slots, size_t capacity, char *key, void *value)
{
	size_t idx = str_hash(key) & (capacity - 1);

	while (slots[idx].key)
		idx = (idx + 1) & (capacity - 1);

	slots[idx].key = key;
	slots[idx].value = value;
}

/* Double the table capacity and re-insert the existing entries. */
static int hashmap_grow(struct hashmap *map)
{
	size_t new_capacity = map->capacity << 1;
	struct hashmap_slot *new_slots;
	size_t i;

	new_slots = (struct hashmap_slot *) calloc(new_capacity, sizeof(*new_slots));
	if (!new_slots)
		return -1;

	for (i = 0; i < map->capacity; i++)
		if (map->slots[i].key)
			hashmap_place(new_slots, new_capacity, map->slots[i].key, map->slots[i].value);

	free(map->slots);
	map->slots = new_slots;
	map->capacity = new_capacity;

	return 0;
}

int hashmap_put(struct hashmap *map, const char *key, void *value)
{
	size_t idx = str_hash(key) & (map->capacity - 1);
	char *copy;

	/* Replace the value if the key is already present. */
	while (map->slots[idx].key) {
		if (!strcmp(map->slots[idx].key, key)) {
			map->slots[idx].value = value;
			return 0;
		}
		idx = (idx + 1) & (map->capacity - 1);
	}

	/* Keep the load factor below 0.5 to bound the probe length. */
	if ((map->count + 1) * 2 > map->capacity) {
		if (hashmap_grow(map) < 0)
			return -1;
	}

	copy = strdup(key);
	if (!copy)
		return -1;

	hashmap_place(map->slots, map->capacity, copy, value);
	map->count++;

	return 0;
}

void *hashmap_get(const struct hashmap *map, const char *key)
{
	size_t idx = str_hash(key) & (map->capacity - 1);

	while (map->slots[idx].key) {
		if (!strcmp(map->slots[idx].key, key))
			return map->slots[idx].value;
		idx = (idx + 1) & (map->capacity - 1);
	}

	return NULL;
}

size_t hashmap_count(const struct hashmap *map)
{
	return map->count;
}
