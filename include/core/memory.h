/* memory.h - memory utils */
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

#include <stdint.h>

#include <core/atomic.h>
#include <core/paging.h>

#define KMEM_BLOCK_LAST	0
#define KMEM_BLOCK_USED	1
#define KMEM_BLOCK_FREE	2
#define KMEM_BLOCK_RESV	3

struct BlockDescriptor {
	uint64_t size : 62;
	uint64_t flags : 2;
} __attribute((packed));

extern const void _kheap_shared;
extern const void _kheap_end;

void meminit(void);

/*
* flushes the tlb entry for a single page (addr)
* addr must tbe 4K page aligned
*/
void tlbflush_addr(void* addr);

/*
* flushes the entire tlb
*/
void tlbflush(void);

void* kmalloc(uint64_t length);
void* _kmalloc(struct BlockDescriptor* heapbase, uint64_t length);

void* kcalloc(uint64_t count, uint64_t length);

void* kzalloc(uint64_t length);

void* krealloc(void* ptr, uint64_t length);
void* kzrealloc(void* ptr, uint64_t length);

void kfree(void* ptr);

#endif /* CORE_MEMORY_H */


