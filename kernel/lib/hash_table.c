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

#include <core/alloc.h>

struct node_t {
	uint64_t key;
	void* value;
	struct node_t* next;
};

struct hash_table_t {
	size_t num_buckets;
	struct node_t* buckets[];
};

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
	struct hash_table_t* table = kmalloc(sizeof(struct hash_table_t) + buckets * sizeof(struct node_t*));

	if (!table) {
		return 0;
	}

	table->num_buckets = buckets;

	for (size_t i = 0; i < buckets; i++) {
		table->buckets[i] = 0;
	}

	return table;
}

void* hash_table_insert(struct hash_table_t* table, uint64_t key, void* value) {
	uint64_t index = bucket_index(table, key);

	struct node_t* node = *find_node(&table->buckets[index], key);

	if (!node) {
		node = kmalloc(sizeof(struct node_t));
		node->next = table->buckets[index];
		node->key = key;
		node->value = 0;
		table->buckets[index] = node;
	}

	void* old = node->value;
	node->value = value;

	return old;
}

void* hash_table_get(struct hash_table_t* table, uint64_t key) {
	uint64_t index = bucket_index(table, key);

	struct node_t* node = *find_node(&table->buckets[index], key);

	if (!node) {
		return 0;
	}

	return node->value;
}

void* hash_table_remove(struct hash_table_t* table, uint64_t key) {
	uint64_t index = bucket_index(table, key);

	struct node_t** node = find_node(&table->buckets[index], key);

	if (!*node) {
		return 0;
	}

	void* old = (*node)->value;
	struct node_t* to_free = *node;
	*node = (*node)->next;

	kfree(to_free);

	return old;
}
