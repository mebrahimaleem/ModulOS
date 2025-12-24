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
#include <core/alloc.h>
#include <core/logging.h>

#include <lib/kmemset.h>

#define ALLOCATION_UNIT	0x200000
#define PAGE_SIZE 			0x1000

// it should not take more than 4GiB to bootstrap mm
#define MAX_BOOTSTRAP	2048

#define INITIAL_VIRTUAL_LIMIT	0xFFFFFFFF80000000

#define ORDER_SIZE(order)	((uint64_t)(PAGE_SIZE * (1 << (uint64_t)order)))

#define SIZE_GIB	(1024 * 1024 * 1024)

extern uint64_t page_frames_num;
extern struct page_frame_t* page_frames;
extern uint64_t virt_limit;
extern struct mm_order_entry_t order_entries[MM_MAX_ORDER];
extern uint8_t _kernel_pend;

static uint64_t used[MAX_BOOTSTRAP];

static void (*early_first_segment)(uint64_t* handle);
static void (*early_next_segment)(uint64_t* handle, struct mem_segment_t* seg);
static uint64_t early_skip;
static uint64_t kernel_limit;

extern uint64_t mem_limit;

void mm_init(
		void (*first_segment)(uint64_t* handle),
		void (*next_segment)(uint64_t* handle, struct mem_segment_t* seg)) {

	kernel_limit = (uint64_t)&_kernel_pend;
	early_first_segment = first_segment;
	early_next_segment = next_segment;
	early_skip = 0;

	// permanent virtual addresses are useful for early memory/paging
	virt_limit = INITIAL_VIRTUAL_LIMIT;

	logging_log_debug("Initializing paging");
	paging_init();
	logging_log_debug("Early paging done");

	// find memory limit
	mem_limit = 0;

	uint64_t handle;
	struct mem_segment_t seg;

	first_segment(&handle);
	for (early_next_segment(&handle, &seg); seg.size || seg.base; next_segment(&handle, &seg)) {
		logging_log_debug("Memory segment 0x%X64 (base) 0x%X64 (size) 0x%X64 (type)",
				(uint64_t)seg.base, (uint64_t)seg.size, (uint64_t)seg.type);
		if (seg.type == MEM_AVL && seg.base + seg.size > mem_limit) mem_limit = seg.base + seg.size;
	}

	logging_log_info("Detected 0x%X64 bytes (0x%X64 GiB) of memory", mem_limit, (uint64_t)(mem_limit / SIZE_GIB));

	// find number of pages
	page_frames_num = (mem_limit + PAGE_SIZE - 1) / PAGE_SIZE;
	const uint64_t frames_size = sizeof(struct page_frame_t) * page_frames_num;

	// allocate pv
	page_frames = (struct page_frame_t*)mm_alloc_pv(frames_size);

	uint64_t i;
	for (i = (uint64_t)page_frames; i < (uint64_t)page_frames + frames_size; i += ALLOCATION_UNIT) {
		paging_early_map_2m(i, mm_early_alloc_2m(), PAGE_PRESENT | PAGE_RW);
	}

	mm_init_dv();
	alloc_init();

	// now bootstrapping mm, paging, and heap is setup

	// setup order entries and frame array
	for (i = 0; i < MM_MAX_ORDER; i++) {
		order_entries[i].free = 0;
		order_entries[i].bitmap = early_kmalloc(page_frames_num / (uint64_t)(8 * (1 << i)));
		kmemset(order_entries[i].bitmap, 0, page_frames_num / (uint64_t)(8 * ( 1 << i)));
	}

	i = early_skip;
	struct mm_free_buddy_t* t;
	first_segment(&handle);
	for (early_next_segment(&handle, &seg); seg.size || seg.base; ) {
		if (seg.type == MEM_AVL) {
			if (seg.base <= kernel_limit && seg.size >= PAGE_SIZE) {
				seg.base += PAGE_SIZE;
				seg.size -= PAGE_SIZE;
				continue;
			}

			if (i > 0 && seg.size >= 2 * ALLOCATION_UNIT) {
				seg.size -= ALLOCATION_UNIT + (seg.base % ALLOCATION_UNIT == 0
						? 0 : (ALLOCATION_UNIT - (seg.base % ALLOCATION_UNIT)));
				seg.base += ALLOCATION_UNIT + (seg.base % ALLOCATION_UNIT == 0
						? 0 : (ALLOCATION_UNIT - (seg.base % ALLOCATION_UNIT)));
				i--;
				continue;
			}

			if (seg.size >= PAGE_SIZE) {
				int8_t order;
				for (order = MM_MAX_ORDER - 1; order >= 0; order--) {
					if (seg.size >= ORDER_SIZE(order) && seg.base % ORDER_SIZE(order) == 0) {
						t = order_entries[order].free;
						order_entries[order].free = early_kmalloc(sizeof(struct mm_free_buddy_t));
						order_entries[order].free->base = seg.base;
						order_entries[order].free->next = t;

						seg.base += ORDER_SIZE(order);
						seg.size -= ORDER_SIZE(order);
						break;
					}
				}

				// if no hit, round to page
				if (order == -1) {
					seg.size -= PAGE_SIZE - (seg.base % PAGE_SIZE);
					seg.base += PAGE_SIZE - (seg.base % PAGE_SIZE);
				}
				continue;
			}
		}

		next_segment(&handle, &seg);
	}

	// account for lost padding
	uint64_t prv_cons = 0;
	uint64_t size;
	for (i = 0; i < early_skip; i++) {
		if (used[i] != prv_cons + ALLOCATION_UNIT && used[i] % ALLOCATION_UNIT != 0) {
				size = ALLOCATION_UNIT - (used[i] % ALLOCATION_UNIT);
				while (size >= PAGE_SIZE) {
					int8_t order;
					for (order = MM_ORDER_2M - 1; order >= 0; order--) {
						if (size >= ORDER_SIZE(order) && used[i] % ORDER_SIZE(order) == 0) {
							t = order_entries[order].free;
							order_entries[order].free = early_kmalloc(sizeof(struct mm_free_buddy_t));
							order_entries[order].free->base = used[i];
							order_entries[order].free->next = t;

							used[i] += ORDER_SIZE(order);
							size -= ORDER_SIZE(order);
							break;
						}
					}

					// if no hit, round to page
					if (order == -1) {
						size -= PAGE_SIZE - (used[i] % PAGE_SIZE);
						used[i] += PAGE_SIZE - (used[i] % PAGE_SIZE);
					}
				}
			}

		prv_cons = used[i];
	}
}

uint64_t mm_early_alloc_2m(void) {

	uint64_t handle, i = early_skip;
	struct mem_segment_t seg;

	if (early_skip == MAX_BOOTSTRAP) {
		panic(PANIC_NO_MEM);
	}

	early_first_segment(&handle);
	for (early_next_segment(&handle, &seg); seg.size || seg.base; ) {
		// ensuring twice the size makes math a lot easier
		if (seg.type == MEM_AVL && seg.size >= 2 * ALLOCATION_UNIT) {
				if (seg.base <= kernel_limit) {
					seg.base += ALLOCATION_UNIT;
					seg.size -= ALLOCATION_UNIT;
					continue;
				}

				if (i--) {
					seg.base += ALLOCATION_UNIT;
					seg.size -= ALLOCATION_UNIT;
					continue;
				}

				used[early_skip++] = seg.base;
				if (seg.base % ALLOCATION_UNIT == 0) return seg.base;
				else return seg.base + ALLOCATION_UNIT - (seg.base % ALLOCATION_UNIT);
		}

		early_next_segment(&handle, &seg);
	}

	panic(PANIC_NO_MEM);
}
