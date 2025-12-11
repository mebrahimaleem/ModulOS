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

#define INITIAL_ALLOC_SIZE		0x200000

#define INITIAL_VIRTUAL_LIMIT	0xFFFFFFFF80000000

uint64_t page_frames_num;
struct page_frame_t* page_frames;

static uint64_t virt_limit; // pv start, alloc backwards

void mm_init_virt() {
	// permanent virtual addresses are useful for early memory/paging
	virt_limit = INITIAL_VIRTUAL_LIMIT;
}

uint64_t mm_alloc_pv(size_t size) {
	if (size % INITIAL_ALLOC_SIZE != 0) {
		size += INITIAL_ALLOC_SIZE - (size % INITIAL_ALLOC_SIZE);
	}

	virt_limit -= size;
	return virt_limit;
}
