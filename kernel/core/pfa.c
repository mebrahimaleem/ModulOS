/* pfa.c - Page frame allocator */
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

#include <core/pfa.h>
#include <core/earlymemory.h>

#define MAX_ORDER	10
#define PAGE_SHIFT 12

// linux style buddy allocator
struct memblock_node_t {
	uint64_t base;
	struct memblock_node_t* next;
};

struct frame_tracker_t {
	struct memblock_node_t* head;
	uint8_t* buddy_map;
};

static struct frame_tracker_t frame_array[MAX_ORDER];
static size_t num_blocks[MAX_ORDER];

void pfa_init(struct memmap_t* memmap, size_t num_memmap) {
	size_t i;
	uint8_t order;
	for (i = 0; i < num_memmap; i++) {
		if (memmap[i].type != MEMTYPE_AVLB) //TODO: handle other memory types
			continue;

		size_t remaining = memmap[i].length;
		uint64_t base = memmap[i].base;
		order = MAX_ORDER;
		uint64_t blocksize = 1ULL << ((uint64_t)order - 1 + PAGE_SHIFT);
		while (remaining) {
			if (remaining < blocksize) {
				if (--order == 0)
					break;
				blocksize = 1ULL << ((uint64_t)order - 1 + PAGE_SHIFT);
				continue;
			}

			struct memblock_node_t* node = kmalloc_early(sizeof(struct memblock_node_t));
			*node = (struct memblock_node_t){
				.base = base,
				.next = frame_array[order - 1].head
			};
			frame_array[order - 1].head = node;
			num_blocks[order - 1]++;

			remaining -= blocksize;
			base += blocksize;
		}
	}

	for (order = 0; order < MAX_ORDER; order++) {
		const uint64_t bitmap_len = (num_blocks[order] + 7) >> 3;
		frame_array[order].buddy_map = kmalloc_early(sizeof(uint8_t) * bitmap_len);
		for (i = 0; i < bitmap_len; i++)
			frame_array[order].buddy_map[i] = 0;
	}
}

static inline uint8_t ilog2_ciel_64(uint64_t x) {
	if (x == 1) return 0;
	return (uint8_t)(sizeof(x) - (uint64_t)__builtin_clzll(x - 1));
}


uint64_t pfa_alloc(size_t num_pages) {
	const uint8_t order = ilog2_ciel_64(num_pages);


}

void pfa_free(uint64_t addr, size_t num_pages) {

}

