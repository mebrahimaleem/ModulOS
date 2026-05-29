/* array_list.c - array list implementation */
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

#include <stdint.h>
#include <stddef.h>

#include <lib/array_list.h>
#include <lib/kmemcpy.h>

#include <core/alloc.h>

struct array_list_t {
	void** buffer;
	void* null;
	size_t cap;
	size_t growth;
	size_t count;
	size_t hint;
};

static void* get_increase(struct array_list_t* list, size_t index) {
	if (index < list->cap) {
		return list->buffer[index];
	}

	void** buffer = kmalloc(sizeof(void*) * list->cap + list->growth);
	kmemcpy(buffer, list->buffer, sizeof(void*) * list->cap);

	for (size_t i = list->cap; i < list->cap + list->growth; i++) {
		buffer[i] = list->null;
	}

	kfree(list->buffer);
	list->buffer = buffer;
	list->cap += list->growth;

	return list->null;
}

struct array_list_t* array_list_alloc(size_t init_cap, size_t growth, void* null) {
	struct array_list_t* list = kmalloc(sizeof(struct array_list_t));

	list->cap = init_cap;
	list->null = null;
	list->growth = growth;
	list->count = list->hint = 0;

	list->buffer = kmalloc(sizeof(void*) * init_cap);

	for (size_t i = 0; i < init_cap; i++) {
		list->buffer[i] = null;
	}

	return list;
}

uint64_t array_list_push(struct array_list_t* list, void* value) {
	while (get_increase(list, list->hint) != list->null) {
		list->hint++;
	}

	list->buffer[list->hint] = value;
	list->count++;
	return list->hint++;
}

void* array_list_get(struct array_list_t* list, uint64_t index) {
	if (index >= list->cap) {
		return list->null;
	}

	return list->buffer[index];
}

void* array_list_remove(struct array_list_t* list, uint64_t index) {
	if (index >= list->cap) {
		return list->null;
	}

	void* ret = list->buffer[index];
	list->buffer[index] = list->null;

	list->count--;

	if (index < list->hint) {
		list->hint = index;
	}

	return ret;
}

void array_list_clear(struct array_list_t* list, void (*free_func)(void*)) {
	for (size_t i = 0; i < list->cap; i++) {
		if (list->buffer[i] != list->null) {
			if (free_func) {
				free_func(list->buffer[i]);
			}
			list->buffer[i] = list->null;
		}
	}

	list->count = 0;
	list->hint = 0;
}

void array_list_free(struct array_list_t* list, void (*free_func)(void*)) {
	array_list_clear(list, free_func);

	kfree(list->buffer);
	kfree(list);
}

struct array_list_t* array_list_dup(struct array_list_t* list, void* (*dup_func)(void*)) {
	struct array_list_t* copy = kmalloc(sizeof(struct array_list_t));

	copy->cap = list->cap;
	copy->null = list->null;
	copy->growth = list->growth;
	copy->count = list->count;
	copy->hint = list->hint;

	copy->buffer = kmalloc(sizeof(void*) * list->cap);

	for (size_t i = 0; i < list->cap; i++) {
		if (list->buffer[i] == list->null) {
			copy->buffer[i] = list->null;
		}
		else {
			copy->buffer[i] = dup_func(list->buffer[i]);
		}
	}

	return copy;
}

uint64_t array_list_count(struct array_list_t* list) {
	return list->count;
}
