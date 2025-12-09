/* mm.h - memory manager interface */
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

#ifndef CORE_MM_H
#define CORE_MM_H

#include <stdint.h>
#include <stddef.h>

#define MM_INITIAL_ALLOC_SIZE	0x200000

struct mem_segment_handle_t;

extern void mm_init(
		void (*next_segment)(struct mem_segment_handle_t** handle),
		struct mem_segment_handle_t* (*first_segment)(void),
		uint64_t (*get_base)(struct mem_segment_handle_t*),
		size_t (*get_size)(struct mem_segment_handle_t*),
		void (*set_base)(struct mem_segment_handle_t*, uint64_t),
		void (*set_size)(struct mem_segment_handle_t*, size_t));

extern uint64_t mm_alloc_frame(void);

extern void mm_free_frame(uint64_t addr);

extern uint64_t mm_alloc_pv(size_t size);

#endif /* CORE_MM_H */

