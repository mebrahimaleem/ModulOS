/* mm_init.h - memory manager initialization interface */
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

#ifndef CORE_MM_INIT_H
#define CORE_MM_INIT_H

#include <stdint.h>
#include <stddef.h>

struct mem_segment_t {
	uint64_t base;
	size_t size;
	enum {
		MEM_AVL,
		MEM_CLM,
		MEM_PRS
	} type;
};

extern void mm_init(
		void (*first_segment)(uint64_t* handle),
		void (*next_segment)(uint64_t* handle, struct mem_segment_t* seg));

extern uint64_t mm_early_alloc_2m(void);

extern uint64_t mm_early_alloc_dv(size_t size);

#endif /* CORE_MM_INIT_H */
