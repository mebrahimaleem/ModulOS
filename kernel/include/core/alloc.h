/* alloc.h - heap allocator interface */
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

#ifndef KERNEL_CORE_ALLOC_H
#define KERNEL_CORE_ALLOC_H

#include <stdint.h>
#include <stddef.h>

enum slab_t {
	SLAB_1,
	SLAB_2,
	SLAB_4,
	SLAB_8,
	SLAB_16,
	SLAB_32,
	SLAB_64,
	SLAB_128,
	SLAB_256,
	SLAB_512,
	SLAB_1K,
	SLAB_2K,
	SLAB_4K,
	SLAB_8K,
	SLAB_16K,
	SLAB_32K,
	SLAB_64K,
	SLAB_128K,
	SLAB_256K,
	SLAB_512K,
	SLAB_1M,
	SLAB_2M,
	SLAB_MAX
};

struct proc_alloc_caches_t {
	uint8_t lock;
	struct cache_node_t* avl;
	struct cache_node_t* slabs[SLAB_MAX];
};

extern void alloc_init(void);

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

#endif /* KERNEL_CORE_ALLOC_H */
