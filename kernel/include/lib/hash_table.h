/* hash_table.h - hash table interface */
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

#ifndef KERNEL_LIB_HASH_TABLE_H
#define KERNEL_LIB_HASH_TABLE_H

#include <stddef.h>
#include <stdint.h>

struct hash_table_t;

extern struct hash_table_t* hash_table_alloc(size_t buckets);

extern void* hash_table_insert(struct hash_table_t* table, uint64_t key, void* value);

extern uint8_t hash_table_get(struct hash_table_t* table, uint64_t key, void** out);

extern uint8_t hash_table_remove(struct hash_table_t* table, uint64_t key, void** out);

extern void hash_table_clear(struct hash_table_t* table, void (*free_func)(void*));

extern void hash_table_free(struct hash_table_t* table, void (*free_func)(void*));

extern void hash_table_copy(struct hash_table_t* table, struct hash_table_t* copy, void* (*dup_func)(void*));

extern void hash_table_resize(struct hash_table_t* table, size_t buckets);

extern size_t hash_table_count(struct hash_table_t* table);

#endif /* KERNEL_LIB_HASH_TABLE_H */

