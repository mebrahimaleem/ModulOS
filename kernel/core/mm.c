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

#define PV_ALLOC_ALIGN	0x200000

uint64_t page_frames_num;
struct page_frame_t* page_frames;

uint64_t virt_limit; // pv start, alloc backwards

struct mm_order_entry_t order_entries[MM_MAX_ORDER];

uint64_t mm_alloc_pv(size_t size) {
	if (size % PV_ALLOC_ALIGN != 0) {
		size += PV_ALLOC_ALIGN - (size % PV_ALLOC_ALIGN);
	}

	virt_limit -= size;
	return virt_limit;
}
