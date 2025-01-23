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

#include <stdint.h>

#include <core/memory.h>
#include <core/panic.h>
#include <core/utils.h>
#include <core/kentry.h>

#include <multiboot2/multiboot2.h>

uint64_t kphint;

struct Blocks32M* kmemblocks32M;

struct BlockDescriptor* volatile kheap_shared;
struct BlockDescriptor* volatile kheap_private;

void kmeminit(void) {
	uint32_t i;
	uint64_t j;

	uint64_t t0;
	uint64_t t1;

	kphint = 0;
	kmemblocks32M = 0;

	/* TODO: optimize kpmembitmap init */
	for (i = 0; i < kavail_memory.length; i++) {
		/* check if block is free */
		if (kavail_memory.entries[i].type != MULTIBOOT2_AVAILRAM)
			continue;

		/* resolve granularity */
		
		/* check edge case */
		if (kavail_memory.entries[i].base_addr + kavail_memory.entries[i].length < 0x2000000)
			continue;

		t0 = kavail_memory.entries[i].base_addr % 0x2000000;

		if (t0 == 0) {
			t0 = kavail_memory.entries[i].base_addr;
			t1 = t0 + kavail_memory.entries[i].length - 0x2000000;
		}
		else {
			t1 = kavail_memory.entries[i].base_addr + kavail_memory.entries[i].length - 0x2000000;
			t0 = kavail_memory.entries[i].base_addr + 0x2000000 - t0;
		}

		
		for (j = t0; j <= t1; j += 0x2000000) {
			kpmembitmap[j / 0x10000000] = 1 << (j % 0x10000000);
		}
	}

	/* mark first 2 gibibytes as taken (by kernel) */
	/* 2 gibibytes * 1024 / 32 / 64 = 8 qwords */
	for (i = 0; i < 8; i++) {
		kpmembitmap[i] = 0;
	}


	/* setup (static for now) shared memory */
	kheap_shared = (struct BlockDescriptor* volatile)&_kheap_shared;
	kheap_private = (struct BlockDescriptor* volatile)&_kheap_private;
	
	/* first block descriptor is reserved and has the maximum size of the heap (half of remaining memory) */
	kheap_shared[0].size = KMEM_BLOCK_SIZEMASK & ((uint64_t)kheap_private - (uint64_t)kheap_shared);
	kheap_shared[0].flags = KMEM_BLOCK_RESV;
	
	/* second block descriptor is reserved and has the current size of the heap (starts at 2MiB) */
	kheap_shared[1].size = 0x200000;
	kheap_shared[1].flags = KMEM_BLOCK_RESV;

	/* third block descriptor */
	kheap_shared[2].size = 0;
	kheap_shared[2].flags = KMEM_BLOCK_FREE;

	//kheap_private->size = kheap_shared->size;
	//kheap_private->flags = KMEM_BLOCK_RESV;
}

/*
* finds a free continious block of 32MiB physical memory of size length
*/
uint64_t kmemfcb(uint64_t length) {
	uint64_t ret = 0;
	uint64_t cont = 0;

	/* first pass */
	for (; kphint < 0x1000000000000; kphint += 0x2000000) {
		if ((kpmembitmap[kphint / 0x10000000] >> (kphint % 0x10000000)) & 1) {
			if (ret == 0) {
				ret = kphint;
				cont = 0;
			}
			
			cont += 0x2000000;
			
			if (cont >= length)
				return ret;	
		}

		ret = 0;
	}

	/* second pass */
	kphint = 0;

	for (; kphint < 0x1000000000000; kphint += 0x2000000) {
		if ((kpmembitmap[kphint / 0x10000000] >> (kphint % 0x10000000)) & 1) {
			if (ret == 0) {
				ret = kphint;
				cont = 0;
			}
			
			cont += 0x2000000;
			
			if (cont >= length)
				return ret;	
		}

		ret = 0;
	}

	return 0; //No memory of length found
}

/*
* maps virtual memory to physical address
* all arguments in bytes and must be 4K page aligned
*/
uint8_t kmapv2p(PML4T_t pml4t, void* vaddr, void* paddr, enum PageGranularity granularity) {
	
}

void* kmmap(PML4T_t pml4t, void* addr, uint64_t flags, uint64_t length) {
	
}

uint8_t kmummap(PML4T_t pml4t, void* addr, uint64_t length) {

}

void* kmalloc(struct BlockDescriptor* volatile heapbase, uint64_t length) {	/* TODO: implement heap mutux (add third reserved descriptor for mutex */
	const uint64_t lim = heapbase[1].size - sizeof(struct BlockDescriptor);

	/* adjust length to force alignment and allocate descriptor */
	const uint64_t alen = length + sizeof(struct BlockDescriptor) + 8 - (length % 8);
	
	void* ret;
	struct BlockDescriptor* volatile block = &heapbase[2];
	uint64_t t0;

	while (alen + (uint64_t)block < lim){
		if (block->flags == KMEM_BLOCK_LAST) {
			/* mark block as used */
			block->flags = KMEM_BLOCK_USED;
			block->size = KMEM_BLOCK_SIZEMASK & alen;
			ret = (void*)((uint64_t)block + sizeof(struct BlockDescriptor));
			block = (struct BlockDescriptor* volatile)((uint64_t)(block) + alen);
			
			/* set next block as free */
			block->size = 0;
			block->flags = KMEM_BLOCK_LAST;

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
				
				return ret;
			}

			/* otherwise overallocate entire block */
			else if (block->size >= alen){
				block->flags = KMEM_BLOCK_USED;
				ret = (void*)((uint64_t)block + sizeof(struct BlockDescriptor));

				return ret;
			}
		}

		/* get next block */
		block = (struct BlockDescriptor* volatile)((uint64_t)(block) + alen);
	}

	/* out of memory */
	
	/* check max mem */
	if (heapbase[1].size + alen + sizeof(struct BlockDescriptor) >= heapbase[0].size) {
		/* TODO: implement errno */
		return 0; /* out of virtual address space (kernel) or max memory per process (userland) */
	}

	const uint64_t newlen = alen > 0x1000 ? alen + 0x1000 - (alen % 0x1000) : 0x1000;

	/* mmmap and recurse */
	if (kmmap(kPML4T, (void*)(heapbase[1].size + (uint64_t)heapbase), 0x3, newlen) == 0) {
		/* TODO: implement errno */
		return 0; /* out of physical memory */
	}
	
	heapbase[1].size += KMEM_BLOCK_SIZEMASK & newlen;
	return kmalloc(heapbase, length);
}

void* kcalloc(struct BlockDescriptor* heapbase, uint64_t count, uint64_t length) {
	const uint64_t alen = count * length;

	void* ret = kmalloc(heapbase, alen);
	kfill(ret, alen, 0);
	return ret;
}

void* krealloc(struct BlockDescriptor* heapbase, void* ptr, uint64_t length) {
	struct BlockDescriptor* volatile block = (struct BlockDescriptor* volatile)((uint64_t)ptr - sizeof(struct BlockDescriptor));
	void* ret;

	if (block->size >= length + sizeof(struct BlockDescriptor)) {
		return ptr;
	}
	else {
		ret = kmalloc(heapbase, length);
		kcopy(ptr, ret, block->size - sizeof(struct BlockDescriptor));
		kfree(ptr);
		return ret;
	}
}

void kfree(void* ptr) {
	struct BlockDescriptor* volatile block = (struct BlockDescriptor* volatile)((uint64_t)ptr - sizeof(struct BlockDescriptor));
	const struct BlockDescriptor* nblk = (struct BlockDescriptor* volatile)((uint64_t)(block) + block->size);

	/* check if next is last */
	if (nblk->flags == KMEM_BLOCK_LAST) {
		block->flags = KMEM_BLOCK_FREE;
		block->size = 0;

		/* TODO: implement page freeing (if needed) */
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

#endif /* CORE_MEMORY_C */
