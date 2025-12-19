/* alloc.c - heap allocator */
/* Copyright (C) 2025  Ebrahim Aleem
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

#include <core/alloc.h>
#include <core/mm.h>
#include <core/mm_init.h>
#include <core/paging.h>

#include <lib/mem_utils.h>

#define ARENA_SIZE	0x200000

#define EXTENT_FREE	0
#define EXTENT_USED	1

struct arena_t {
	struct arena_t* next;
	uint64_t* hint;
	uint64_t left;
	uint64_t arena[(ARENA_SIZE
			- sizeof(struct arena_t*)
			- sizeof(uint64_t)
			- sizeof(uint64_t)) / sizeof(uint64_t)];
};

_Static_assert(sizeof(struct arena_t) == ARENA_SIZE, "arena must be 2M");

static struct arena_t* base;
static struct arena_t* head;

void alloc_init(void) {
	base = (struct arena_t*)mm_alloc_dv(MM_ORDER_2M);
	head = base;
	paging_early_map_2m((uint64_t)base, mm_early_alloc_2m(), PAGE_PRESENT | PAGE_RW);

	// preallocate prevents mm/alloc deadlock
	memset(base, 0, ARENA_SIZE);
	base->next = (struct arena_t*)mm_alloc_dv(MM_ORDER_2M);
	paging_early_map_2m((uint64_t)base->next, mm_early_alloc_2m(), PAGE_PRESENT | PAGE_RW);
	base->left = sizeof(base->arena);
	base->hint = &base->arena[0];
}

//TODO: allow chosing arena
void* kmalloc(size_t size) {
	// TODO: reclaim freed
	
	// round to 8
	if (size % sizeof(uint64_t) != 0) {
		size += sizeof(uint64_t) - (size % sizeof(uint64_t));
	}

	// TODO: check if hint is free
	if ((uint64_t)head->hint + size >= (uint64_t)head + ARENA_SIZE) {	
		head = head->next;
		memset(head, 0, ARENA_SIZE);
		head->next = (struct arena_t*)mm_alloc_dv(MM_ORDER_2M);
		paging_map((uint64_t)head->next, mm_alloc(MM_ORDER_2M), PAGE_PRESENT | PAGE_RW, PAGE_2M);
		head->left = sizeof(head->arena);
		head->hint = &head->arena[0];
	}

	void* const ret = &head->hint[1];

	head->hint[0] = EXTENT_USED | size;
	head->hint = (uint64_t*)((uint64_t)&head->hint[1] + size);
	head->left -= size + sizeof(uint64_t);

	return ret;
}

void* early_kmalloc(size_t size) {
	// TODO: reclaim freed
	
	// round to 8
	if (size % sizeof(uint64_t) != 0) {
		size += sizeof(uint64_t) - (size % sizeof(uint64_t));
	}

	// TODO: check if hint is free
	if ((uint64_t)head->hint + size >= (uint64_t)head + ARENA_SIZE) {	
		head = head->next;
		memset(head, 0, ARENA_SIZE);
		head->next = (struct arena_t*)mm_alloc_dv(MM_ORDER_2M);
		paging_early_map_2m((uint64_t)head->next, mm_early_alloc_2m(), PAGE_PRESENT | PAGE_RW);
		head->left = sizeof(head->arena);
		head->hint = &head->arena[0];
	}

	void* const ret = &head->hint[1];

	head->hint[0] = EXTENT_USED | size;
	head->hint = (uint64_t*)((uint64_t)&head->hint[1] + size);
	head->left -= size + sizeof(uint64_t);

	return ret;
}
