/* mm.h - memory manager interface */
/* Copyright (C) 2025-2026  Ebrahim Aleem
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

#ifndef KERNEL_CORE_MM_H
#define KERNEL_CORE_MM_H

#include <stdint.h>
#include <stddef.h>

#define CANON_HIGH			0xFFFF800000000000
#define VIRTUAL_LIMIT		IDENT_BASE
#define SIZE_2M					0x200000

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

uint64_t mm_alloc_p(size_t size);
uint64_t mm_alloc_v(size_t size);

uint64_t mm_alloc_p2m(void);
uint64_t mm_alloc_v2m(void);

void mm_free_p(uint64_t addr, size_t size);
void mm_free_v(uint64_t addr, size_t size);

void mm_free_p2m(uint64_t addr);
void mm_free_v2m(uint64_t addr);

void mm_defrag(void);

#endif /* KERNEL_CORE_MM_H */

