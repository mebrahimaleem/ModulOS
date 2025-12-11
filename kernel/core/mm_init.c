/* mm_init.c - memory manager initialization */
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

#include <core/mm_init.h>
#include <core/mm.h>
#include <core/paging.h>
#include <core/panic.h>

#define ALLOCATION_UNIT	0x200000
#define PAGE_SIZE 			0x1000

// it should not take more than 8GiB to bootstrap mm
#define MAX_BOOTSTRAP	4096

extern uint64_t page_frames_num;
extern struct page_frame_t* page_frames;

static uint64_t used[MAX_BOOTSTRAP];

static void (*early_first_segment)(uint64_t* handle);
static void (*early_next_segment)(uint64_t* handle, struct mem_segment_t* seg);
static uint64_t early_skip;

void mm_init(
		void (*first_segment)(uint64_t* handle),
		void (*next_segment)(uint64_t* handle, struct mem_segment_t* seg)) {

	early_first_segment = first_segment;
	early_next_segment = next_segment;
	early_skip = 0;

	mm_init_virt();

	paging_init(mm_early_alloc_2m());

	// find memory limit
	uint64_t mem_limit = 0;

	uint64_t handle;
	struct mem_segment_t seg;

	first_segment(&handle);
	for (early_next_segment(&handle, &seg); seg.size || seg.base; next_segment(&handle, &seg)) {
		if (seg.base + seg.size > mem_limit) mem_limit = seg.base + seg.size;
	}

	// find number of pages
	page_frames_num = (mem_limit + PAGE_SIZE - 1) / PAGE_SIZE;
	const uint64_t frames_size = sizeof(struct page_frame_t) * page_frames_num;

	// allocate pv
	page_frames = (struct page_frame_t*)mm_alloc_pv(frames_size);

	uint64_t i;
	for (i = (uint64_t)page_frames; i < (uint64_t)page_frames + frames_size; i += ALLOCATION_UNIT) {
		paging_early_map_2m(i, mm_early_alloc_2m(), PAGE_PRESENT | PAGE_RW);
	}
}

uint64_t mm_early_alloc_2m() {

	uint64_t handle, i = early_skip + 1;
	struct mem_segment_t seg;

	early_first_segment(&handle);
	for (early_next_segment(&handle, &seg); seg.size || seg.base; ) {
		// ensuring twice the size makes math a lot easier
		if (seg.size >= 2 * ALLOCATION_UNIT) {
				if (!--i) {
					seg.base += ALLOCATION_UNIT;
					seg.size -= ALLOCATION_UNIT;
					continue;
				}
				else {
					used[early_skip++] = seg.base;
					if (seg.base % ALLOCATION_UNIT == 0) return seg.base;
					else return seg.base + ALLOCATION_UNIT - (seg.base % ALLOCATION_UNIT);
				}
		}

		early_next_segment(&handle, &seg);
	}

	panic(PANIC_NO_MEM);
}
