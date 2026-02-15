/* paging.h - paging managger interface */
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

#ifndef KERNEL_CORE_PAGING_H
#define KERNEL_CORE_PAGING_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_PRESENT	0x1
#define PAGE_RW				0x2
#define PAT_MMIO_4K		0x98

#define PAGE_BASE_MASK	0xFFFFFFFFFFFFF000	

#define PAGE_SIZE_4K		0x1000
#define PAGE_SIZE_2M		0x200000
#define PAGE_SIZE_1G		0x40000000
#define KERNEL_VMA 			0xFFFFFFFF80000000
#define IDENT_BASE			0xFFFFFF0000000000

enum page_size_t {
	PAGE_4K,
	PAGE_2M,
	PAGE_1G,
	_PAGE_512G, // reserved for internal
};

extern void paging_init(void);

extern uint64_t paging_map(uint64_t vaddr, uint64_t paddr, uint8_t flg, enum page_size_t page_size);
extern void paging_unmap(uint64_t vaddr, enum page_size_t page_size);
extern uint64_t paging_ident(uint64_t paddr);


#endif /* KERNEL_CORE_PAGING_H */
