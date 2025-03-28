/* memory.c - memory utils */
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

#ifndef CORE_MEMORY_C
#define CORE_MEMORY_C

// granularity
#define PMEMBITS_G			0x2000000

// blocks per byte
#define PMEMBPERBYTE		0x10000000

// max pmem
#define PMEMMAX					0x1000000000000

#define P4KSIZE					0x1000
#define P2MSIZE					0x200000
#define P1GSIZE					0x40000000

#include <stdint.h>

#include <core/memory.h>
#include <core/atomic.h>
#include <core/panic.h>
#include <core/utils.h>
#include <core/kentry.h>

#include <core/serial.h>

#include <multiboot2/multiboot2.h>

uint64_t kphint;

PML4T_t volatile kPML4T;

uint8_t* volatile pmembitmap;

struct Blocks32M* kmemblocks32M;

struct BlockDescriptor* volatile kheap_shared;
struct BlockDescriptor* volatile kheap_private;

struct PagesFree* volatile kpagesfree;

MutexHandle pagingGlobalMutex;
MutexHandle memoryGlobalLock;

void meminit(void) {
	uint32_t i;
	uint64_t j;

	uint64_t t0;
	uint64_t t1;

	pmembitmap = &_pmembitmap;

	pagingGlobalMutex = kcreateMutex();
	memoryGlobalLock = kcreateMutex();

	kpagesfree = 0;

	kphint = 0x80000000; //skip kernel memory
	kmemblocks32M = 0;

	/* TODO: optimize kpmembitmap init */
	/* generate free memory bitmap */
	for (i = 0; i < kavail_memory.length; i++) {
		/* check if block is free */
		if (kavail_memory.entries[i].type != MULTIBOOT2_AVAILRAM)
			continue;
		
		/* check underflow edge case (and block too small case while we are at)*/
		if (kavail_memory.entries[i].length < PMEMBITS_G)
			continue;

		/* resolve granularity */
		
		t0 = kavail_memory.entries[i].base_addr % PMEMBITS_G;

		/*
		 * t0 is pmem starting address
		 * t1 is pmem ending address
		 */

		/* base aligned */
		if (t0 == 0) {
			t0 = kavail_memory.entries[i].base_addr;
			t1 = t0 + kavail_memory.entries[i].length - PMEMBITS_G; // subtract granularity to force entire block to be free, lte check still passes
		}
		/* base not aligned */
		else {
			t1 = kavail_memory.entries[i].base_addr + kavail_memory.entries[i].length - PMEMBITS_G; // will skip trailing memory since we cannot track it
			t0 = kavail_memory.entries[i].base_addr + PMEMBITS_G - t0;
		}
		
		for (j = t0; j <= t1; j += PMEMBITS_G) {
			pmembitmap[j / PMEMBPERBYTE] |= 1 << (j % PMEMBPERBYTE / PMEMBITS_G);
		}
	}

	/* setup memory */
	kheap_shared = (struct BlockDescriptor* volatile)&_kheap_shared;
	kheap_private = (struct BlockDescriptor* volatile)&_kheap_private;
	
	/* shared memory block descriptors */
	/* first block descriptor is reserved and has the maximum size of the heap (half of remaining memory) */
	kheap_shared[0].size = KMEM_BLOCK_SIZEMASK & ((uint64_t)kheap_private - (uint64_t)kheap_shared);
	kheap_shared[0].flags = KMEM_BLOCK_RESV;

	/* second block descriptor has current size of the heap (starting at 2MiB) */
	kheap_shared[1].size = P2MSIZE;
	kheap_shared[1].flags = KMEM_BLOCK_RESV;
	
	/* third block descriptor is heap mutex handle*/
	((uint64_t*)kheap_shared)[2] = kcreateMutex();

	/* fourth is pointer to heap extension */
	//TODO

	/* fifth block descriptor is free (last) */
	kheap_shared[4].size = 0;
	kheap_shared[4].flags = KMEM_BLOCK_LAST;

	/* now init the private heap */
	if (kmmap(kPML4T, (uint8_t*)kheap_private, KMEM_PAGE_PRESENT | KMEM_PAGE_WRITE, P2MSIZE) == 1)
		panic(KPANIC_NOMEM);

	/* first is max size */
	kheap_private[0].size = kheap_shared->size;
	kheap_private[0].flags = KMEM_BLOCK_RESV;

	/* second is current size */
	kheap_private[1].size = P2MSIZE;
	kheap_private[1].flags = KMEM_BLOCK_RESV;

	/* third is mutex handle */
	((uint64_t*)kheap_private)[2] = kcreateMutex();

	/* fourth is pointer to heap extension */
	//TODO

	/* fifth is last */
	kheap_private[4].size = 0;
	kheap_private[4].flags = KMEM_BLOCK_LAST;
}

/*
* finds a free continious block of 32MiB physical memory of size length
*/
uint64_t memfcb(uint64_t length) {
	uint64_t ret = 0;
	uint64_t cont = 0;

	/* first pass, uses hint */
	for (; kphint < PMEMMAX; kphint += PMEMBITS_G) {
		if ((pmembitmap[kphint / PMEMBPERBYTE] & (1 << (kphint % PMEMBPERBYTE / PMEMBITS_G))) != 0) {
			if (ret == 0) {
				ret = kphint;
				cont = 0;
			}
			
			cont += PMEMBITS_G;
			
			if (cont >= length)
				return ret;	
			continue;
		}

		ret = 0;
	}

	/* second pass */
	kphint = 0x80000000; //skip kernel memory

	for (; kphint < PMEMMAX; kphint += PMEMBITS_G) {
		if (((pmembitmap[kphint / PMEMBPERBYTE] >> (kphint % PMEMBPERBYTE / PMEMBITS_G)) & 1) != 0) {
			if (ret == 0) {
				ret = kphint;
				cont = 0;
			}
			
			cont += PMEMBITS_G;
			
			if (cont >= length)
				return ret;	

			continue;
		}

		ret = 0;
	}


	return 0; //No memory of length found
}

/*
* maps virtual memory to physical address
* all arguments in bytes and must be 4K page aligned
* undefined behavior when overwriting previous entries with a different granularity
*/
void mapv2p(PML4T_t volatile pml4t, void* vaddr, void* paddr, uint8_t flags, enum PageGranularity granularity) {
	kacquireMutex(pagingGlobalMutex);

	const uint64_t l4 = 0x1FF & ((uint64_t)vaddr / 0x8000000000); // identify 512GiB
	const uint64_t l3 = 0x1FF & ((uint64_t)vaddr / 0x0040000000); // identify 1GiB
	const uint64_t l2 = 0x1FF & ((uint64_t)vaddr / 0x200000); // identify 2MiB
	const uint64_t l1 = 0x1FF & ((uint64_t)vaddr / 0x1000); // identify 2GiB

	uint64_t t0;

	if (((uint64_t)pml4t[l4] & 1) == 0) { // page not present
		t0 = (uint64_t)kcalloc(kheap_shared, 0x1000, 1);
		if (t0 == 0) {
			panic(KPANIC_NOMEM);
		}
		pml4t[l4] = (PDPT_t)(calculatePaddr(pml4t, t0) | flags);
	}

	const PDPT_t volatile p4 = (PDPT_t volatile)((uint64_t)pml4t[l4] & PS_ADDR_MASK);

	if (granularity == PAGE_GRANULARITY_1G) {
		p4[l3] = (PDT_t)((uint64_t)paddr | flags | KMEM_PAGEFLAG_PS);
		tlbflush();
		kreleaseMutex(pagingGlobalMutex);
		return;
	}

	if (((uint64_t)p4[l3] & 1) == 0) { // page not present
		t0 = (uint64_t)kcalloc(kheap_shared, 0x1000, 1);
		if (t0 == 0) {
			panic(KPANIC_NOMEM);
		}
		p4[l3] = (PDT_t)(calculatePaddr(pml4t, t0) | flags);
	}

	const PDT_t p3 = (PDT_t volatile)((uint64_t)p4[l3] & PS_ADDR_MASK);

	if (granularity == PAGE_GRANULARITY_2M) {
		p3[l2] = (PT_t)((uint64_t)paddr | flags | KMEM_PAGEFLAG_PS);
		tlbflush();
		kreleaseMutex(pagingGlobalMutex);
		return;
	}
	
	if (((uint64_t)p3[l2] & 1) == 0) { // page not present
		t0 = (uint64_t)kcalloc(kheap_shared, 0x1000, 1);
		if (t0 == 0) {
			panic(KPANIC_NOMEM);
		}
		p3[l2] = (PT_t)(calculatePaddr(pml4t, t0) | flags);
	}

	const PT_t volatile p2 = (PT_t volatile)((uint64_t)p3[l2] & PS_ADDR_MASK);

	p2[l1] = (uint64_t)paddr | flags;
	tlbflush_addr(vaddr);
	kreleaseMutex(pagingGlobalMutex);
}

void unmapv2p(PML4T_t pml4t, void* vaddr, enum PageGranularity granularity) {
	//TODO: rewrite with masks
	kacquireMutex(pagingGlobalMutex);
	const uint64_t l4 = 0x1FF & ((uint64_t)vaddr / 0x8000000000); // identify 512GiB
	const uint64_t l3 = 0x1FF & ((uint64_t)vaddr / 0x0040000000); // identify 1GiB
	const uint64_t l2 = 0x1FF & ((uint64_t)vaddr / 0x200000); // identify 2MiB
	const uint64_t l1 = 0x1FF & ((uint64_t)vaddr / 0x1000); // identify 2GiB

	if (((uint64_t)pml4t[l4] & 1) == 0) { // page not present
		kreleaseMutex(pagingGlobalMutex);
		return; // already unmaped
	}

	if (granularity == PAGE_GRANULARITY_1G) {
		pml4t[l4][l3] = (PDT_t)(0);
		tlbflush();
		kreleaseMutex(pagingGlobalMutex);
		return;
	}

	if (((uint64_t)pml4t[l4][l3] & 1) == 0) { // page not present
		kreleaseMutex(pagingGlobalMutex);
		return; // already unmaped
	}

	if (granularity == PAGE_GRANULARITY_2M) {
		pml4t[l4][l3][l2] = (PT_t)(0);
		tlbflush();
		kreleaseMutex(pagingGlobalMutex);
		return;
	}
	
	if (((uint64_t)pml4t[l4][l3][l2] & 1) == 0) { // page not present
		kreleaseMutex(pagingGlobalMutex);
		return; // already unmaped
	}

	pml4t[l4][l3][l2][l1] = 0;
	tlbflush_addr(vaddr);
	kreleaseMutex(pagingGlobalMutex);
}

uint64_t kmmap(PML4T_t pml4t, void* addr, uint8_t flags, uint64_t length) {
	kacquireMutex(memoryGlobalLock);
	uint64_t i = 0;
	struct PagesFree* t0;
	uint8_t* t1;

	while (i < length && kpagesfree != 0) {
		// start by using free pages
		if (kpagesfree->length > length - i) {
			// use only part of free pages
			t1 = kpagesfree->addr;
			kpagesfree->addr += length - i;
			kpagesfree->length -= length - i;
			while (i < length) {
				if (length - i >= P1GSIZE) {
					//1G
					mapv2p(pml4t, (uint8_t*)addr + i, t1, flags, PAGE_GRANULARITY_1G);
					i += P1GSIZE;
					t1 += P1GSIZE;
				}
				else if (length - i >= P2MSIZE) {
					//2M
					mapv2p(pml4t, (uint8_t*)addr + i, t1, flags, PAGE_GRANULARITY_2M);
					i += P2MSIZE;
					t1 += P2MSIZE;
				}
				else {
					//4K
					mapv2p(pml4t, (uint8_t*)addr + i, t1, flags, PAGE_GRANULARITY_4K);
					i += P4KSIZE;
					t1 += P4KSIZE;
				}
			}

			// at this point all memory should be allocated
			break;
		}
		else {
			// use entire free pages
			while (kpagesfree->length > 0) {
				if (kpagesfree->length >= P1GSIZE) {
					//1G
					mapv2p(pml4t, (uint8_t*)addr + i, kpagesfree->addr, flags, PAGE_GRANULARITY_1G);
					i += P1GSIZE;
					kpagesfree->addr += P1GSIZE;
					kpagesfree->length -= P1GSIZE;
				}
				else if (length - i >= P2MSIZE) {
					//2M
					mapv2p(pml4t, (uint8_t*)addr + i, kpagesfree->addr, flags, PAGE_GRANULARITY_2M);
					i += P2MSIZE;
					kpagesfree->addr += P2MSIZE;
					kpagesfree->length -= P2MSIZE;
				}
				else {
					//4K
					mapv2p(pml4t, (uint8_t*)addr + i, kpagesfree->addr, flags, PAGE_GRANULARITY_4K);
					i += P4KSIZE;
					kpagesfree->addr += P4KSIZE;
					kpagesfree->length -= P4KSIZE;
				}
			}
			t0 = kpagesfree;
			kpagesfree = kpagesfree->next;
			kfree(t0);
		}
	}

	if (i == length) {
		kreleaseMutex(memoryGlobalLock);
		return 0; // sucesfull
	}

	// next try finding new memory
	t1 = (uint8_t*)memfcb(length - i);

	if (t1 == 0) {
		//TODO: errno
		//TODO: try smaller continious block
		kreleaseMutex(memoryGlobalLock);
		return 1;
	}

	while (i < length) {
		if (length - i >= P1GSIZE) {
			//1G
			mapv2p(pml4t, (uint8_t*)addr + i, t1, flags, PAGE_GRANULARITY_1G);
			i += P1GSIZE;
			t1 += P1GSIZE;
		}
		else if (length - i >= P2MSIZE) {
			//2M
			mapv2p(pml4t, (uint8_t*)addr + i, t1, flags, PAGE_GRANULARITY_2M);
			i += P2MSIZE;
			t1 += P2MSIZE;
		}
		else {
			//4K
			mapv2p(pml4t, (uint8_t*)addr + i, t1, flags, PAGE_GRANULARITY_4K);
			i += P4KSIZE;
			t1 += P4KSIZE;
		}
	}

	// all memory now allocated

	//TODO: add leftover memory to kpagesfree
	
	//TODO: mark 32MiB block as dirty
	kreleaseMutex(memoryGlobalLock);
	return 0;
}

uint8_t kmummap(PML4T_t pml4t, void* addr, uint64_t length) {
	//TODO: implement
}

void* kmalloc(struct BlockDescriptor* volatile heapbase, uint64_t length) {
	kacquireMutex(((uint64_t*)heapbase)[2]);
	const uint64_t lim = heapbase[1].size - sizeof(struct BlockDescriptor);

	/* adjust length to force alignment and allocate descriptor */
	const uint64_t alen = length + sizeof(struct BlockDescriptor) + 8 - (length % 8);
	
	void* ret;
	struct BlockDescriptor* volatile block = &heapbase[4];
	uint64_t t0;

	while (alen + (uint64_t)block < lim + (uint64_t)heapbase) {
		if (block->flags == KMEM_BLOCK_LAST) {
			/* mark block as used */
			block->flags = KMEM_BLOCK_USED;
			block->size = KMEM_BLOCK_SIZEMASK & alen;
			ret = (void*)((uint64_t)block + sizeof(struct BlockDescriptor));
			block = (struct BlockDescriptor* volatile)((uint64_t)(block) + alen);
			
			/* set next block as free */
			block->size = 0;
			block->flags = KMEM_BLOCK_LAST;

			kreleaseMutex(((uint64_t*)heapbase)[2]);
			return ret;
		}

		else if (block->flags == KMEM_BLOCK_FREE) {
			/* check if there is room for multiple blocks */
			if (block->size >= alen + sizeof(struct BlockDescriptor)) {
				t0 = block->size;

				block->flags = KMEM_BLOCK_USED;
				block->size = KMEM_BLOCK_SIZEMASK & alen;
				ret = (void*)((uint64_t)block + sizeof(struct BlockDescriptor));

				block->flags = KMEM_BLOCK_FREE;
				block->size = KMEM_BLOCK_SIZEMASK & (t0 - alen);
				
				kreleaseMutex(((uint64_t*)heapbase)[2]);
				return ret;
			}

			/* otherwise overallocate entire block */
			else if (block->size >= alen){
				block->flags = KMEM_BLOCK_USED;
				ret = (void*)((uint64_t)block + sizeof(struct BlockDescriptor));

				kreleaseMutex(((uint64_t*)heapbase)[2]);
				return ret;
			}
		}

		/* get next block */
		block = (struct BlockDescriptor* volatile)((uint64_t)(block) + alen);
	}

	/* out of memory */
	//TODO: attempt allocating memory
	//TODO: attempt memory extension
	
	/* TODO: implement errno */
	kreleaseMutex(((uint64_t*)heapbase)[2]);
	return 0; /* out of virtual address space */
}

void* kcalloc(struct BlockDescriptor* heapbase, uint64_t count, uint64_t length) {
	const uint64_t alen = count * length;

	void* ret = kmalloc(heapbase, alen);
	if (ret == 0) {
		return 0;
	}
	kfill(ret, alen, 0);
	return ret;
}

void* krealloc(struct BlockDescriptor* heapbase, void* ptr, uint64_t length) {
	struct BlockDescriptor* volatile block = (struct BlockDescriptor* volatile)((uint64_t)ptr - sizeof(struct BlockDescriptor));
	void* ret;

	//TODO: peek if next if free or last
	if (block->size >= length + sizeof(struct BlockDescriptor)) {
		return ptr;
	}
	else {
		ret = kmalloc(heapbase, length);
		if (ret == 0) {
			return 0;
		}
		kcopy(ptr, ret, block->size - sizeof(struct BlockDescriptor));
		kfree(ptr);
		return ret;
	}
}

void kfree(void* ptr) {
	//TODO: call unmap
	struct BlockDescriptor* volatile block = (struct BlockDescriptor* volatile)((uint64_t)ptr - sizeof(struct BlockDescriptor));
	const struct BlockDescriptor* nblk = (struct BlockDescriptor* volatile)((uint64_t)(block) + block->size);

	/* check if next is last */
	if (nblk->flags == KMEM_BLOCK_LAST) {
		block->flags = KMEM_BLOCK_FREE;
		block->size = 0;
	}
	
	/* check if next is free */
	else if (nblk->flags == KMEM_BLOCK_FREE) {
		block->flags = KMEM_BLOCK_FREE;
		block->size += nblk->size;
	}

	else {
		block->flags = KMEM_BLOCK_FREE;
	}
}

uint64_t calculatePaddr(PML4T_t pml4t, uint64_t vaddr) {
	const uint64_t l4 = 0x1FF & ((uint64_t)vaddr / 0x8000000000); // identify 512GiB
	const uint64_t l3 = 0x1FF & ((uint64_t)vaddr / 0x0040000000); // identify 1GiB
	const uint64_t l2 = 0x1FF & ((uint64_t)vaddr / 0x200000); // identify 2MiB
	const uint64_t l1 = 0x1FF & ((uint64_t)vaddr / 0x1000); // identify 2GiB

	const PDPT_t volatile p4 = (PDPT_t volatile)((uint64_t)pml4t[l4] & PS_ADDR_MASK);

	if (((uint64_t)p4[l3] & KMEM_PAGEFLAG_PS) == KMEM_PAGEFLAG_PS) {
		return (uint64_t)p4[l3] & PS_ADDR_MASK;
	}

	const PDT_t p3 = (PDT_t volatile)((uint64_t)p4[l3] & PS_ADDR_MASK);

	if (((uint64_t)p3[l2] & KMEM_PAGEFLAG_PS) == KMEM_PAGEFLAG_PS) {
		return (uint64_t)p3[l2] & PS_ADDR_MASK;
	}

	const PT_t volatile p2 = (PT_t volatile)((uint64_t)p3[l2] & PS_ADDR_MASK);

	return p2[l1] & PS_ADDR_MASK;
}
#endif /* CORE_MEMORY_C */
