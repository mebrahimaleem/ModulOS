/* paging.h - paging related routines */
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

enum PageGranularity {
	PAGE_GRANULARITY_4K,
	PAGE_GRANULARITY_2M,
	PAGE_GRANULARITY_1G,
};

struct paging_order {
	uint64_t base;
	struct paging_order* next;
};

extern const void _kheap_static;
extern const void STATIC_HEAP_SIZE;

extern volatile PML4T_t kPML4T;

void initpaging(void);

void* claimblock(uint64_t size);
void freeblock(void* addr, uint64_t size);

void* allocpaging(void);
void freepaging(void);

uint64_t calculatePaddr(PML4T_t pml4t, uint64_t vaddr);

/*
* maps virtual memory to physical address
* all arguments in bytes and must be 4K page aligned
*/
void mapv2p(PML4T_t pml4t, void* vaddr, void* paddr, uint8_t flags, enum PageGranularity granularity);
void unmapv2p(PML4T_t pml4t, void* vaddr, enum PageGranularity granularity);

void* kmmap(PML4T_t pml4t, void* addr, uint8_t flags, uint64_t length);
uint8_t kmunmap(PML4T_t pml4t, void* addr, uint64_t length);

#endif /* CORE_PAGING_H */

