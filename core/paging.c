/* paging.c - paging related routines */
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

#ifndef CORE_PAGING_C
#define CORE_PAGING_C

#include <stdint.h>

#include <core/atomic.h>
#include <core/kentry.h>
#include <core/memory.h>
#include <core/paging.h>
#include <core/panic.h>

#include <multiboot2/multiboot2.h>

#define ORDER_4K		0x0
#define ORDER_8K		0x1
#define ORDER_16K		0x2
#define ORDER_32K		0x3
#define ORDER_64K		0x4
#define ORDER_128K	0x5
#define ORDER_256K	0x6
#define ORDER_512K	0x7
#define ORDER_1M		0x8
#define ORDER_2M		0x9
#define ORDER_4M		0xa

#define SIZE_4K		0x1000
#define SIZE_8K		0x2000
#define SIZE_16K	0x4000
#define SIZE_32K	0x8000
#define SIZE_64K	0x10000
#define SIZE_128K	0x20000
#define SIZE_256K	0x40000
#define SIZE_512K	0x80000
#define SIZE_1M		0x100000
#define SIZE_2M		0x200000
#define SIZE_4M		0x400000

#define FLAG_MASK				0xFFFFFFFFFFFFFFE1
#define FLAGS_ONLY_MASK	0x1F

#define COMPONENT_2M		0xFFFFFFFFFFE00000	
#define COMPONENT_1G		0xFFFFFFFFC0000000

#define ALIGN_TO(addr, align) ((uint64_t)(addr + align - 1) & ~(uint64_t)(align - 1))

#define APPEND_ORDER(x, ordptr) \
	t0 = _kmalloc(static_heap, sizeof(struct paging_order)); \
	t0->base = x; \
	t0->next = ordptr; \
	ordptr = t0;

#define TRY_CLAIM(order, sz) \
	else if (mem_orders[order] != 0 && size <= sz) { \
		t = mem_orders[order]->base; \
		mem_orders[order] = mem_orders[order]->next; \
		leftover = sz - size; \
		allocated = sz; \
	}

#define TRY_TRACK(order, sz) \
	base = ALIGN_TO(kavail_memory.entries[i].base_addr, sz); \
	if (base + sz <= lim) { \
		APPEND_ORDER(base, mem_orders[order]); \
		kavail_memory.entries[i].base_addr += sz; \
		kavail_memory.entries[i].length -= sz; \
		continue; \
	}

static struct paging_order* mem_orders[11];
volatile PML4T_t kPML4T;

struct BlockDescriptor* static_heap;
StaticMutexHandle memblock_mutex;

void initpaging() {
	static_heap = (struct BlockDescriptor*)&_kheap_static;

	/* shared memory block descriptors */
	/* first block descriptor is reserved and has the maximum size of the heap */
	static_heap[0].size = KMEM_BLOCK_SIZEMASK & (uint64_t)&STATIC_HEAP_SIZE;
	static_heap[0].flags = KMEM_BLOCK_RESV;

	/* second block descriptor has current size of the heap */
	static_heap[1].size = KMEM_BLOCK_SIZEMASK & (uint64_t)&STATIC_HEAP_SIZE;
	static_heap[1].flags = KMEM_BLOCK_RESV;
	
	/* third block descriptor is heap mutex handle*/
	((uint64_t*)static_heap)[2] = kcreateStaticMutex();

	/* fourth is reserved for future use */

	/* fifth block descriptor is free (last) */
	static_heap[4].size = 0;
	static_heap[4].flags = KMEM_BLOCK_LAST;

	memblock_mutex = kcreateStaticMutex();

	struct paging_order* t0;

	mem_orders[0] =
		mem_orders[1] = 
		mem_orders[2] = 
		mem_orders[3] = 
		mem_orders[4] = 
		mem_orders[5] = 
		mem_orders[6] = 
		mem_orders[7] = 
		mem_orders[8] = 
		mem_orders[9] = 
		mem_orders[10] = 0;

	/* track free memory */
	for (uint64_t i = 0; i < kavail_memory.length; ) {

		/* check if block is free */
		if (kavail_memory.entries[i].type != MULTIBOOT2_AVAILRAM) {
			i++;
			continue;
		}

		/* skip first 6MiB (reserved for kernel */
		if (kavail_memory.entries[i].base_addr < 0x600000)
			kavail_memory.entries[i].base_addr = 0x600000;

		/* multiboot2 sometimes misses ISA hole even when it exists, so skip over it */
		if (kavail_memory.entries[i].base_addr >= 0xc0000000 && kavail_memory.entries[i].base_addr < 0x100000000) {
			kavail_memory.entries[i].base_addr = 0x100000000;
		}
		
		const uint64_t lim = kavail_memory.entries[i].base_addr + kavail_memory.entries[i].length;
		uint64_t base;

		/* try in decreasing order for optimized memory usage */
		TRY_TRACK(ORDER_4M, SIZE_4M);
		TRY_TRACK(ORDER_2M, SIZE_2M);
		TRY_TRACK(ORDER_1M, SIZE_1M);
		TRY_TRACK(ORDER_512K, SIZE_512K);
		TRY_TRACK(ORDER_256K, SIZE_256K);
		TRY_TRACK(ORDER_128K, SIZE_128K);
		TRY_TRACK(ORDER_64K, SIZE_64K);
		TRY_TRACK(ORDER_32K, SIZE_32K);
		TRY_TRACK(ORDER_16K, SIZE_16K);
		TRY_TRACK(ORDER_8K, SIZE_8K);
		TRY_TRACK(ORDER_4K, SIZE_4K);

		/* otherwise block is too small */
		i++;
	}
}

void* claimblock(uint64_t size) {
	kacquireStaticMutex(memblock_mutex);
	uint64_t t;
	uint64_t leftover;
	uint64_t allocated;
	struct paging_order* t0;

	/* start from smallest to minimize wasted memory */
	if (mem_orders[ORDER_4K] != 0 && size <= SIZE_4K) {
		t = mem_orders[ORDER_4K]->base;
		mem_orders[ORDER_4K] = mem_orders[ORDER_4K]->next;
		leftover = 0; //cant do leftovers with 4K
		allocated = 0;
	}

	TRY_CLAIM(ORDER_8K, SIZE_8K)
	TRY_CLAIM(ORDER_16K, SIZE_16K)
	TRY_CLAIM(ORDER_32K, SIZE_32K)
	TRY_CLAIM(ORDER_64K, SIZE_64K)
	TRY_CLAIM(ORDER_128K, SIZE_128K)
	TRY_CLAIM(ORDER_256K, SIZE_256K)
	TRY_CLAIM(ORDER_512K, SIZE_512K)
	TRY_CLAIM(ORDER_1M, SIZE_1M)
	TRY_CLAIM(ORDER_2M, SIZE_2M)
	TRY_CLAIM(ORDER_4M, SIZE_4M)
	/* no support for allocations greater than 4M */

	else {
		kreleaseStaticMutex(memblock_mutex);
		return 0;
	}

	/* deal with leftover memory */
	while (leftover > SIZE_4K) {
		/* intentionally break into little chunks in anticipation of small future allocations */
		/* breaking leftovers into smaller chunks will lead to fewer leftovers in the long-term */
		if (leftover >= SIZE_2M) {
			APPEND_ORDER(allocated, mem_orders[ORDER_2M]);
			leftover -= SIZE_2M;
			allocated += SIZE_2M;
		}
		else if (leftover >= SIZE_256K) {
			APPEND_ORDER(allocated, mem_orders[ORDER_256K]);
			leftover -= SIZE_256K;
			allocated += SIZE_256K;
		}
		else if (leftover >= SIZE_32K) {
			APPEND_ORDER(allocated, mem_orders[ORDER_32K]);
			leftover -= SIZE_32K;
			allocated += SIZE_32K;
		}
		else {
			APPEND_ORDER(allocated, mem_orders[ORDER_4K]);
			leftover -= SIZE_4K;
			allocated += SIZE_4K;
		}
	}

	kreleaseStaticMutex(memblock_mutex);
	return (void*)t;
}

void freeblock(void* addr, uint64_t size) {
	//TODO: implement
}

/* returns a physical address (which is also a valid virtual address since first 512GiB is identity mapped */
void* allocpaging() {
	void* ret = claimblock(SIZE_4K);
	if (ret == 0) {
		panic(KPANIC_NOMEM);
	}
	return ret;
}

void freepaging() {
	//TODO: implement
}

/*
* maps virtual memory to physical address
* all arguments in bytes and must be 4K page aligned
*/
void mapv2p(volatile PML4T_t pml4t, void* vaddr, void* paddr, uint8_t flags, enum PageGranularity granularity) {
	const uint64_t l4 = 0x1FF & ((uint64_t)vaddr / 0x8000000000); // identify 512GiB
	const uint64_t l3 = 0x1FF & ((uint64_t)vaddr / 0x0040000000); // identify 1GiB
	const uint64_t l2 = 0x1FF & ((uint64_t)vaddr / 0x200000); // identify 2MiB
	const uint64_t l1 = 0x1FF & ((uint64_t)vaddr / 0x1000); // identify 2GiB

	uint64_t t0;

	if (((uint64_t)pml4t[l4] & 1) == 0) { // page not present
		t0 = (uint64_t)allocpaging();
		pml4t[l4] = (PDPT_t)(t0 | flags);
	}

	const volatile PDPT_t p4 = (volatile PDPT_t )(((uint64_t)pml4t[l4] & PS_ADDR_MASK) + KMEM_ID_OFF);

	if (granularity == PAGE_GRANULARITY_1G) {
		p4[l3] = (PDT_t)((uint64_t)paddr | flags | KMEM_PAGEFLAG_PS);
		tlbflush();
		return;
	}

	if (((uint64_t)p4[l3] & 1) == 0) { // page not present
		t0 = (uint64_t)allocpaging();
		p4[l3] = (PDT_t)(t0 | flags);
	}

	if (((uint64_t)p4[l3] & KMEM_PAGEFLAG_PS) == KMEM_PAGEFLAG_PS) { // wrong granularity
		paging_splitStruct(pml4t, vaddr, PAGE_GRANULARITY_1G);
	}

	const PDT_t p3 = (volatile PDT_t )(((uint64_t)p4[l3] & PS_ADDR_MASK) + KMEM_ID_OFF);

	if (granularity == PAGE_GRANULARITY_2M) {
		p3[l2] = (PT_t)((uint64_t)paddr | flags | KMEM_PAGEFLAG_PS);
		tlbflush();
		return;
	}
	
	if (((uint64_t)p3[l2] & 1) == 0) { // page not present
		t0 = (uint64_t)allocpaging();
		p3[l2] = (PT_t)(t0 | flags);
	}

	if (((uint64_t)p4[l3] & KMEM_PAGEFLAG_PS) == KMEM_PAGEFLAG_PS) { // wrong granularity
		paging_splitStruct(pml4t, vaddr, PAGE_GRANULARITY_2M);
	}

	const volatile PT_t p2 = (volatile PT_t )(((uint64_t)p3[l2] & PS_ADDR_MASK) + KMEM_ID_OFF);

	p2[l1] = (uint64_t)paddr | flags;
	tlbflush_addr(vaddr);
}

void paging_changeFlags(PML4T_t pml4t, void* vaddr, uint8_t flags, enum PageGranularity granularity) {
	const uint64_t l4 = 0x1FF & ((uint64_t)vaddr / 0x8000000000); // identify 512GiB
	const uint64_t l3 = 0x1FF & ((uint64_t)vaddr / 0x0040000000); // identify 1GiB
	const uint64_t l2 = 0x1FF & ((uint64_t)vaddr / 0x200000); // identify 2MiB
	const uint64_t l1 = 0x1FF & ((uint64_t)vaddr / 0x1000); // identify 2GiB

	const volatile PDPT_t p4 = (volatile PDPT_t )(((uint64_t)pml4t[l4] & PS_ADDR_MASK) + KMEM_ID_OFF);

	if (granularity == PAGE_GRANULARITY_1G) {
		*(volatile uint64_t*)&p4[l3] &= FLAG_MASK;
		*(volatile uint64_t*)&p4[l3] |= flags;
		tlbflush();
		return;
	}

	if (((uint64_t)p4[l3] & KMEM_PAGEFLAG_PS) == KMEM_PAGEFLAG_PS) { // wrong granularity
		paging_splitStruct(pml4t, vaddr, PAGE_GRANULARITY_1G);
	}

	const PDT_t p3 = (volatile PDT_t )(((uint64_t)p4[l3] & PS_ADDR_MASK) + KMEM_ID_OFF);

	if (granularity == PAGE_GRANULARITY_2M) {
		*(volatile uint64_t*)&p3[l2] &= FLAG_MASK;
		*(volatile uint64_t*)&p3[l2] |= flags;
		tlbflush();
		return;
	}
	
	if (((uint64_t)p4[l3] & KMEM_PAGEFLAG_PS) == KMEM_PAGEFLAG_PS) { // wrong granularity
		paging_splitStruct(pml4t, vaddr, PAGE_GRANULARITY_2M);
	}

	const volatile PT_t p2 = (volatile PT_t )(((uint64_t)p3[l2] & PS_ADDR_MASK) + KMEM_ID_OFF);

	*(volatile uint64_t*)&p2[l1] &= FLAG_MASK;
	*(volatile uint64_t*)&p2[l1] |= flags;
	tlbflush_addr(vaddr);
}

void paging_splitStruct(PML4T_t pml4t, void* vaddr, enum PageGranularity granularity) {
	const uint64_t l4 = 0x1FF & ((uint64_t)vaddr / 0x8000000000); // identify 512GiB
	const uint64_t l3 = 0x1FF & ((uint64_t)vaddr / 0x0040000000); // identify 1GiB
	const uint64_t l2 = 0x1FF & ((uint64_t)vaddr / 0x200000); // identify 2MiB

	const volatile PDPT_t p4 = (volatile PDPT_t )(((uint64_t)pml4t[l4] & PS_ADDR_MASK) + KMEM_ID_OFF);
	uint16_t i;

	if (granularity == PAGE_GRANULARITY_1G) {
		uint64_t paddr = (uint64_t)p4[l3] & PS_ADDR_MASK;
		const uint8_t flags = (uint64_t)p4[l3] & FLAGS_ONLY_MASK;
		/* clear the entry, then map it again with finer granularity */
		p4[l3] = 0;
		vaddr = (void*)((uint64_t)vaddr & COMPONENT_1G);
		for (i = 0; i < 512; i++) {
			mapv2p(pml4t, vaddr, (void*)paddr, flags, PAGE_GRANULARITY_2M);
			vaddr = (void*)((uint64_t)vaddr + SIZE_2M);
			paddr += SIZE_2M;
		}
		tlbflush();
		return;
	}

	const PDT_t p3 = (volatile PDT_t )(((uint64_t)p4[l3] & PS_ADDR_MASK) + KMEM_ID_OFF);

	if (granularity == PAGE_GRANULARITY_2M) {
		uint64_t paddr = (uint64_t)p3[l2] & PS_ADDR_MASK;
		const uint8_t flags = (uint64_t)p3[l2] & FLAGS_ONLY_MASK;
		/* clear the entry, then map it again with finer granularity */
		p3[l2] = 0;
		vaddr = (void*)((uint64_t)vaddr & COMPONENT_2M);
		for (i = 0; i < 512; i++) {
			mapv2p(pml4t, vaddr, (void*)paddr, flags, PAGE_GRANULARITY_4K);
			vaddr = (void*)((uint64_t)vaddr + SIZE_4K);
			paddr += SIZE_4K;
		}
		tlbflush();
		return;
	}

	/* cant split 4K */
}

void unmapv2p(PML4T_t pml4t, void* vaddr, enum PageGranularity granularity) {
	//TODO: implement
}

void* kmmap(PML4T_t pml4t, void* addr, uint8_t flags, uint64_t length) {
	uint64_t vaddr = (uint64_t)addr;
	uint64_t paddr;
	uint8_t nofragmented = 1;

	/* round length to page boundary */
	length = ALIGN_TO(length, SIZE_4K);
	while (length > 0) {
		/* allocate from largest to smallest */
		if (nofragmented && length >= SIZE_2M) {
			paddr = (uint64_t)claimblock(SIZE_2M);
			if (paddr == 0) {
				/* try smaller, fragmented allocation */
				nofragmented = 0;
				continue;
			}
			mapv2p(pml4t, (void*)vaddr, (void*)paddr, flags, PAGE_GRANULARITY_2M);
			vaddr += SIZE_2M;
			length -= SIZE_2M;
		}
		else {
			paddr = (uint64_t)claimblock(SIZE_4K);
			if (paddr == 0) {
				/* out of memory */
				return (void*)1;
			}
			mapv2p(pml4t, (void*)vaddr, (void*)paddr, flags, PAGE_GRANULARITY_4K);
			vaddr += SIZE_4K;
			length -= SIZE_4K;
		}
	}

	return addr;
}

uint8_t kmunmap(PML4T_t pml4t, void* addr, uint64_t length) {
	//TODO: implement
}

uint64_t calculatePaddr(PML4T_t pml4t, uint64_t vaddr) {
	//TODO: check for no page
	const uint64_t l4 = 0x1FF & ((uint64_t)vaddr / 0x8000000000); // identify 512GiB
	const uint64_t l3 = 0x1FF & ((uint64_t)vaddr / 0x0040000000); // identify 1GiB
	const uint64_t l2 = 0x1FF & ((uint64_t)vaddr / 0x200000); // identify 2MiB
	const uint64_t l1 = 0x1FF & ((uint64_t)vaddr / 0x1000); // identify 4K

	const volatile PDPT_t p4 = (volatile PDPT_t )(((uint64_t)pml4t[l4] & PS_ADDR_MASK) + KMEM_ID_OFF);

	if (((uint64_t)p4[l3] & KMEM_PAGEFLAG_PS) == KMEM_PAGEFLAG_PS) {
		return ((uint64_t)p4[l3] & PS_ADDR_MASK) | (vaddr & 0xFFFFFFF);
	}

	const PDT_t p3 = (volatile PDT_t )(((uint64_t)p4[l3] & PS_ADDR_MASK) + KMEM_ID_OFF);

	if (((uint64_t)p3[l2] & KMEM_PAGEFLAG_PS) == KMEM_PAGEFLAG_PS) {
		return ((uint64_t)p3[l2] & PS_ADDR_MASK) | (vaddr & 0xFFFFF);
	}

	const volatile PT_t p2 = (volatile PT_t )(((uint64_t)p3[l2] & PS_ADDR_MASK) + KMEM_ID_OFF);

	return (p2[l1] & PS_ADDR_MASK) | (vaddr & 0xFFF);
}

#endif /* CORE_PAGING_C */

