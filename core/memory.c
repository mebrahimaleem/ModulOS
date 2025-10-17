/* memory.c - kernel memory */
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

#include <core/memory.h>

#define MOD8_MASK	(size_t)0x7

extern uint8_t _kheap[];

struct early_heap_header_t {
	size_t size;
	enum {
		USED,
		FREE,
		LAST
	} type;
} __attribute__((aligned(8)));

static volatile struct early_heap_header_t* early_heap_base =
	(struct early_heap_header_t*)&_kheap[0];

void memory_init_early() {
	*early_heap_base = (volatile struct early_heap_header_t){.type = LAST};
}

void* kmalloc_early(size_t size) {
	volatile struct early_heap_header_t* i = early_heap_base;

	// enforce 8 byte alignment
	size = (size + 7) & ~MOD8_MASK;

	while (1) {
		switch (i->type) {
			case LAST:
				i->size = size;
				i->type = USED;
				void* tmp = (void*)(i + 1); // pointer arithmetic adds sizeof(*i)
				i = (volatile struct early_heap_header_t*)((uint8_t*)i + i->size);
				i->type = LAST;
				return tmp;
			case FREE:
				if (i->size >= size) {
					i->type = USED;
					return (void*)(i + 1); // pointer arithmetic adds sizeof(*i)
				}
				__attribute__((fallthrough));
			default:
				i = (volatile struct early_heap_header_t*)((uint8_t*)i + i->size);
				break;
		}
	}
}

void kfree_early(void* ptr) {
	volatile struct early_heap_header_t* i = (volatile struct early_heap_header_t*)ptr - 1;
	i->type = FREE;

	// don't collate since allocation is lazy and does not optimize for size
}
