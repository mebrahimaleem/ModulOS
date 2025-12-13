/* early_mem.c - early memory allocator */
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

#include <core/early_mem.h>
#include <core/mm_init.h>
#include <core/mm.h>
#include <core/paging.h>

#define SIZE_2M	0x200000

struct mem_list_t {
	uint64_t base;
	struct mem_list_t* next;
};

static struct mem_list_t* mem_list;
static struct mem_list_t* mem_next;
static struct mem_list_t root;
static uint64_t mem_offset;

void early_mem_init() {
	mem_list = &root;

	mem_list->base = mm_early_alloc_dv(SIZE_2M);
	mem_offset = 0;
	paging_early_map_2m(mem_list->base, mm_early_alloc_2m(), PAGE_PRESENT | PAGE_RW);
	mem_next = early_kmalloc(sizeof(struct mem_list_t));
}

void* early_kmalloc(size_t size) {
	if (mem_offset + size > SIZE_2M) {
		mem_next->next = mem_list;
		mem_list = mem_next;

		mem_list->base = mm_early_alloc_dv(SIZE_2M);
		mem_offset = 0;
		paging_early_map_2m(mem_list->base, mm_early_alloc_2m(), PAGE_PRESENT | PAGE_RW);
		mem_next = early_kmalloc(sizeof(struct mem_list_t));
	}

	mem_offset += size;
	return (void*)(mem_offset - size + mem_list->base);
}

void early_mem_discard(void) {
	// unmap pages, reinit of dv discards heap

	// TODO: copy mem_list to new heap, then discard
}
