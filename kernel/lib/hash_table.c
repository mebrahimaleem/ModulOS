/* hash_table.c - hash table implementation */
/* Copyright (C) 2026  Ebrahim Aleem
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>
*/

#include <stddef.h>
#include <stdint.h>

#include <lib/hash_table.h>
#include <lib/kmemset.h>

#include <core/alloc.h>

struct node_t {
	uint64_t key;
	void* value;
	struct node_t* next;
};

struct hash_table_t {
	size_t num_buckets;
	size_t num_elems;
	struct node_t** buckets;
};

static void* dup(void* value) {
	return value;
}

static inline uint64_t bucket_index(struct hash_table_t* table, uint64_t key) {
	return key % table->num_buckets;
}

static struct node_t** find_node(struct node_t** bucket, uint64_t key) {
	for (; *bucket; bucket = &(*bucket)->next) {
		if ((*bucket)->key == key) {
			break;
		}
	}

	return bucket;
}

struct hash_table_t* hash_table_alloc(size_t buckets) {
	struct hash_table_t* table = kmalloc(sizeof(struct hash_table_t));

	if (!table) {
		return 0;
	}

	table->num_buckets = buckets;
	table->num_elems = 0;
	table->buckets = kmalloc(sizeof(struct node_t*) * buckets);

	kmemset(table->buckets, 0, sizeof(struct node_t*) * buckets);

	return table;
}

void* hash_table_insert(struct hash_table_t* table, uint64_t key, void* value) {
	uint64_t index = bucket_index(table, key);

	struct node_t* node = *find_node(&table->buckets[index], key);

	if (!node) {
		node = kmalloc(sizeof(struct node_t));
		node->next = table->buckets[index];
		node->key = key;
		node->value = value;
		table->buckets[index] = node;
		table->num_elems++;
	}

	void* old = node->value;
	node->value = value;

	return old;
}

uint8_t hash_table_get(struct hash_table_t* table, uint64_t key, void** out) {
	uint64_t index = bucket_index(table, key);

	struct node_t* node = *find_node(&table->buckets[index], key);

	if (!node) {
		return 0;
	}

	*out = node->value;
	return 1;
}

uint8_t hash_table_remove(struct hash_table_t* table, uint64_t key, void** out) {
	uint64_t index = bucket_index(table, key);

	struct node_t** node = find_node(&table->buckets[index], key);

	if (!*node) {
		return 0;
	}

	*out = (*node)->value;
	struct node_t* to_free = *node;
	*node = (*node)->next;

	kfree(to_free);

	table->num_elems--;

	return 1;
}

void hash_table_clear(struct hash_table_t* table, void (*free_func)(void*)) {
	for (size_t i = 0; i < table->num_buckets; i++) {
		struct node_t* next;
		for (struct node_t* node = table->buckets[i]; node; node = next) {
			next = node->next;
			if (free_func) {
				free_func(node->value);
			}
			kfree(node);
		}

		table->buckets[i] = 0;
	}

	table->num_elems = 0;
}

void hash_table_free(struct hash_table_t* table, void (*free_func)(void*)) {
	hash_table_clear(table, free_func);

	kfree(table->buckets);
	kfree(table);
}

void hash_table_copy(struct hash_table_t* table, struct hash_table_t* copy, void* (*dup_func)(void*)) {
	for (size_t i = 0; i < table->num_buckets; i++) {
		for (struct node_t* node = table->buckets[i]; node; node = node->next) {
			hash_table_insert(copy, node->key, dup_func(node->value));
		}
	}
}

void hash_table_resize(struct hash_table_t* table, size_t buckets) {
	struct hash_table_t temp = *table;

	table->num_buckets = buckets;
	table->num_elems = 0;
	table->buckets = kmalloc(sizeof(struct node_t*) * buckets);
	kmemset(table->buckets, 0, sizeof(struct node_t*) * buckets);

	hash_table_copy(&temp, table, dup);

	kfree(temp.buckets);
}

size_t hash_table_count(struct hash_table_t* table) {
	return table->num_elems;
}
