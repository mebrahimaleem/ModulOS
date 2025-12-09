/* mm.c - memory manager */
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

#include <core/mm.h>
#include <core/paging.h>
#include <core/panic.h>

#include <lib/mem_utils.h>

#define PAGE_SIZE				0x1000
#define PD_ENTRY_SIZE 	0x200000
#define PDPT_ENTRY_SIZE 0x40000000

#define FRAME_USED	0x0
#define FRAME_FREE	0x1

#define INITIAL_VIRTUAL_LIMIT	0xFFFFFFFF80000000

#define INITIAL_ALLOC_SIZE	0x200000

#define MAX_SUPPORTED_FRAMES	0x3F800000

struct page_frame_t {
	uint8_t flg;
} __attribute__((packed));

extern uint8_t _kernel_pend;

static struct page_frame_t* page_array;

static uint64_t virt_limit; // pv start

void mm_init(
		void (*next_segment)(struct mem_segment_handle_t**),
		struct mem_segment_handle_t* (*first_segment)(void),
		uint64_t (*get_base)(struct mem_segment_handle_t*),
		size_t (*get_size)(struct mem_segment_handle_t*),
		void (*set_base)(struct mem_segment_handle_t*, uint64_t),
		void (*set_size)(struct mem_segment_handle_t*, size_t)) {

	virt_limit = INITIAL_VIRTUAL_LIMIT;

	uint64_t base;
	uint64_t size;

	uint64_t reclaim_base_pg = 0;
	uint64_t reclaim_size_pg;

	uint64_t paging_base = 0;

	struct mem_segment_handle_t* handle = first_segment();
	while (handle != 0) {
		base = get_base(handle);
		size = get_size(handle);

		if (base % INITIAL_ALLOC_SIZE == 0 && size >= INITIAL_ALLOC_SIZE) {
			paging_base = base;
			set_base(handle, base + INITIAL_ALLOC_SIZE);
			set_size(handle, size - INITIAL_ALLOC_SIZE);
			break;
		}

		reclaim_size_pg = INITIAL_ALLOC_SIZE - (base % INITIAL_ALLOC_SIZE);
		if (size >= INITIAL_ALLOC_SIZE + reclaim_size_pg) {
			reclaim_base_pg = base;
			paging_base = base + reclaim_size_pg;
			set_base(handle, base + reclaim_size_pg + INITIAL_ALLOC_SIZE);
			set_size(handle, size - reclaim_size_pg - INITIAL_ALLOC_SIZE);
			break;
		}

		next_segment(&handle);
	}

	if (paging_base) {
		paging_init(paging_base);
	}
	else {
		panic(PANIC_NO_MEM);
	}

	uint64_t reclaim_base_fr = 0;
	uint64_t reclaim_size_fr;

	uint64_t max_base = 0;
	uint64_t max_size = 0;
	uint64_t mem_limit = 0;

	handle = first_segment();
	while (handle != 0) {
		base = get_base(handle);
		size = get_size(handle);

		if (base + size > mem_limit) {
			mem_limit = base + size;
		}

		if (size > max_size) {
			max_base = base;
			max_size = size;
		}

		next_segment(&handle);
	}

	if (!max_base) {
		panic(PANIC_NO_MEM);
	}

	// roundup to page
	uint64_t frames_size = sizeof(struct page_frame_t) * ((mem_limit + PAGE_SIZE - 1) / PAGE_SIZE);

	if (frames_size > MAX_SUPPORTED_FRAMES) {
		// TODO: maybe support more memory
		frames_size = MAX_SUPPORTED_FRAMES;
	}

	// align to boundary
	if (max_base % INITIAL_ALLOC_SIZE != 0) {
		max_size -= INITIAL_ALLOC_SIZE - (max_base % INITIAL_ALLOC_SIZE);
		max_base += INITIAL_ALLOC_SIZE - (max_base % INITIAL_ALLOC_SIZE);
	}

	if (max_size < frames_size) {
		panic(PANIC_NO_MEM);
	}

	page_array = (struct page_frame_t*)mm_alloc_pv(frames_size);

	for (uint64_t i = 0; i < frames_size; i += INITIAL_ALLOC_SIZE) {
		paging_map_2m((uint64_t)page_array + i, max_base + i, PAGE_PRESENT | PAGE_RW);
	}

	memset(page_array, 0, frames_size);

	handle = first_segment();
	while (handle != 0) {
		base = get_base(handle);
		size = get_size(handle);

		if (base % PAGE_SIZE != 0) {
			size -= PAGE_SIZE - (base % PAGE_SIZE);
			base += PAGE_SIZE - (base % PAGE_SIZE);
		}

		for (uint64_t i = base / PAGE_SIZE; size >= PAGE_SIZE; i++) {
			if (i < (uint64_t)&_kernel_pend / PAGE_SIZE) {
				continue;
			}

			page_array[i].flg |= FRAME_FREE;
			size -= PAGE_SIZE;
		}

		next_segment(&handle);
	}
}

uint64_t mm_alloc_pv(size_t size) {
	virt_limit -= size;
	return virt_limit;

	// TODO: check pv-dv collision
}
