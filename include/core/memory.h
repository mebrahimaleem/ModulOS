/* memory.h - kernel memory interface */
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

#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

#include <stddef.h>

struct memmap_t {
	uint64_t base;
	size_t length;
	enum {
		MEMTYPE_AVLB = 1,
		MEMTYPE_ACPI = 3,
		MEMTYPE_PRES = 4,
		MEMTYPE_DEFC = 5
	} type;
};

extern void* kheap_base;

extern void memory_init_early(void);

extern void* kmalloc_early(size_t size);

extern void kfree_early(void* ptr);

#endif /* CORE_MEMORY_H */
