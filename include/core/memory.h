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

#define KMEM_BLOCK_LAST	0
#define KMEM_BLOCK_USED	1
#define KMEM_BLOCK_FREE	2
#define KMEM_BLOCK_RESV	3

#define KMEM_BLOCK_SIZEMASK	0x3FFFFFFFFFFFFFFF
#define PS_ADDR_MASK				0xFFFFFFFFFF000

#define KMEM_PAGEFLAG_PS		0x80

#define KMEM_PAGE_PRESENT		0x1
#define KMEM_PAGE_WRITE			0x2

#define KMEM_ID_OFF					0x0

typedef uint64_t**** PML4T_t;
typedef uint64_t*** PDPT_t;
typedef uint64_t** PDT_t;
typedef uint64_t* PT_t;

struct Blocks32M {
	uint16_t allocated;
	uint64_t bitmap[128];
	struct Blocks32M* next;
};

struct PagesFree {
	uint8_t* addr;
	uint64_t length;
	struct PagesFree* next;
};

struct BlockDescriptor {
	uint64_t size : 62;
	uint64_t flags : 2;
} __attribute((packed));

enum PageGranularity {
	PAGE_GRANULARITY_4K,
	PAGE_GRANULARITY_2M,
	PAGE_GRANULARITY_1G,
};

extern PML4T_t volatile kPML4T;

extern uint8_t _pmembitmap;
extern const void _kheap_shared;
extern const void _kheap_private;
extern struct BlockDescriptor* volatile kheap_shared;
extern struct BlockDescriptor* volatile kheap_private;

void meminit(void);

/*
* finds free continious blocks of 32MiB physical memory of at least size length
*/
uint64_t memfcb(uint64_t length);

/*
* flushes the tlb entry for a single page (addr)
* addr must tbe 4K page aligned
*/
void tlbflush_addr(void* addr);

/*
* flushes the entire tlb
*/
void tlbflush(void);

/*
* maps virtual memory to physical address
* all arguments in bytes and must be 4K page aligned
*/
void mapv2p(PML4T_t pml4t, void* vaddr, void* paddr, uint8_t flags, enum PageGranularity granularity);
void _mapv2p(PML4T_t pml4t, void* vaddr, void* paddr, uint8_t flags, enum PageGranularity granularity, uint8_t use_mut);

void unmapv2p(PML4T_t pml4t, void* vaddr, enum PageGranularity granularity);

uint64_t kmmap(PML4T_t pml4t, void* addr, uint8_t flags, uint64_t length);
uint64_t _kmmap(PML4T_t pml4t, void* addr, uint8_t flags, uint64_t length, uint8_t use_mut);

uint8_t kmummap(PML4T_t pml4t, void* addr, uint64_t length);

void* kmalloc(struct BlockDescriptor* heapbase, uint64_t length);
void* _kmalloc(struct BlockDescriptor* heapbase, uint64_t length, uint8_t use_mut);

void* kcalloc(struct BlockDescriptor* heapbase, uint64_t count, uint64_t length);
void* _kcalloc(struct BlockDescriptor* heapbase, uint64_t count, uint64_t length, uint8_t use_mut);

void* krealloc(struct BlockDescriptor* heapbase, void* ptr, uint64_t length);

void kfree(void* ptr);

uint64_t calculatePaddr(PML4T_t pml4t, uint64_t vaddr);

#endif /* CORE_MEMORY_H */


