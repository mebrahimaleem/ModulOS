/* paging.h - paging managger interface */
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

#ifndef CORE_PAGING_H
#define CORE_PAGING_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_PRESENT	0x1
#define PAGE_RW				0x2
#define PAT_MMIO_4K		0x98

enum page_size_t {
	PAGE_4K,
	PAGE_2M,
	PAGE_1G
};

extern void paging_init(void);

extern void paging_map(uint64_t vaddr, uint64_t paddr, uint8_t flg, enum page_size_t page_size);

extern void paging_early_map_2m(uint64_t vaddr, uint64_t paddr, uint8_t flg);

#endif /* CORE_PAGING_H */
