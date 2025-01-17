/* tags.h - multiboot2 tags */
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

#ifndef MULTIBOOT2_TAGS_H
#define MULTIBOOT2_TAGS_H

#define MB2TAG_TYPE_END			0
#define MB2TAG_TYPE_MEMMAP	6

#include <stdint.h>

struct mb2tag_memmap_entry {
	uint64_t base_addr;
	uint64_t length;
	uint32_t type;
	uint32_t res;
} __attribute((packed));

struct mb2tag_fixed {
	uint32_t total_size;
	uint32_t res;
} __attribute((packed));

struct mb2tag_start {
	uint32_t type;
	uint32_t size;
} __attribute((packed));

struct mb2tag_memmap {
	uint32_t type;
	uint32_t size;
	uint32_t entry_size;
	uint32_t version;
	struct mb2tag_memmap_entry entries [];
} __attribute((packed));


#endif /* MULTIBOOT2_TAGS_H */
