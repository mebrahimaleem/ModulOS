/* kentry.h - kernel entry point */
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

#ifndef CORE_KENTRY_H
#define CORE_KENTRY_H

#include <stdint.h>

#include <core/memory.h>

#include <multiboot2/tags.h>

struct avail_memory_t {
	struct mb2tag_memmap_entry* entries;
	uint32_t length;
};

extern struct avail_memory_t kavail_memory;

extern volatile uint64_t k0PML4T;

void kentry(uint32_t mb2tag_ptr, uint32_t mb2magic);


#endif /* CORE_KENTRY_H */

