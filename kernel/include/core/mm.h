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

#ifndef KERNEL_CORE_MM_H
#define KERNEL_CORE_MM_H

#include <stdint.h>
#include <stddef.h>

enum mm_order_t {
	MM_ORDER_4K,
	MM_ORDER_8K,
	MM_ORDER_16K,
	MM_ORDER_32K,
	MM_ORDER_64K,
	MM_ORDER_128K,
	MM_ORDER_256K,
	MM_ORDER_512K,
	MM_ORDER_1M,
	MM_ORDER_2M,
	MM_MAX_ORDER
};

struct page_frame_t {
	uint8_t flg;
} __attribute__((packed));

struct mm_free_buddy_t {
	uint64_t base;
	struct mm_free_buddy_t* next;
};

struct mm_order_entry_t {
	struct mm_free_buddy_t* free;
	uint8_t* bitmap;
};

extern uint64_t mm_alloc(enum mm_order_t order);

extern uint64_t mm_alloc_pv(size_t size);

extern void mm_init_dv(void);

extern uint64_t mm_alloc_dv(enum mm_order_t order);

#endif /* KERNEL_CORE_MM_H */

