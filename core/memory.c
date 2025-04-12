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

#include <core/paging.h>
#include <core/utils.h>
#include <core/memory.h>

#define SIZE_2M		0x200000

struct BlockDescriptor* shared_heap;

void meminit() {
	shared_heap = (struct BlockDescriptor*)&_kheap_shared;

	/* shared memory block descriptors */
	/* first block descriptor is reserved and has the maximum size of the heap */
	shared_heap[0].size = (KMEM_BLOCK_SIZEMASK) & ((uint64_t)&_kheap_end - (uint64_t)&_kheap_shared);
	shared_heap[0].flags = KMEM_BLOCK_RESV;

	/* second block descriptor has current size of the heap (2MiB) */
	shared_heap[1].size = KMEM_BLOCK_SIZEMASK & SIZE_2M;
	shared_heap[1].flags = KMEM_BLOCK_RESV;
	
	/* third block descriptor is heap mutex handle*/
	((uint64_t*)shared_heap)[2] = kcreateStaticMutex();

	/* fourth is reserved for future use */

	/* fifth block descriptor is free (last) */
	shared_heap[4].size = 0;
	shared_heap[4].flags = KMEM_BLOCK_LAST;
}


inline void* kmalloc(uint64_t length) {
	return _kmalloc(shared_heap, length);
}

void* _kmalloc(struct BlockDescriptor* heapbase, uint64_t length) {
	kacquireStaticMutex(((uint64_t*)heapbase)[2]);
	const uint64_t lim = heapbase[1].size - sizeof(struct BlockDescriptor);

	/* adjust length to force alignment and allocate descriptor */
	const uint64_t alen = length + sizeof(struct BlockDescriptor) + 8 - (length % 8);
	
	void* ret;
	volatile struct BlockDescriptor* block = &heapbase[4];
	uint64_t t0;

	while (alen + (uint64_t)block < lim + (uint64_t)heapbase) {
		if (block->flags == KMEM_BLOCK_LAST) {
			/* mark block as used */
			block->flags = KMEM_BLOCK_USED;
			block->size = KMEM_BLOCK_SIZEMASK & alen;
			ret = (void*)((uint64_t)block + sizeof(struct BlockDescriptor));
			block = (volatile struct BlockDescriptor* )((uint64_t)(block) + alen);
			
			/* set next block as free */
			block->size = 0;
			block->flags = KMEM_BLOCK_LAST;

			kreleaseStaticMutex(((uint64_t*)heapbase)[2]);
			return ret;
		}

		else if (block->flags == KMEM_BLOCK_FREE) {
			/* check if there is room for multiple blocks */
			if (block->size >= alen + sizeof(struct BlockDescriptor)) {
				t0 = block->size;

				block->flags = KMEM_BLOCK_USED;
				block->size = KMEM_BLOCK_SIZEMASK & alen;
				ret = (void*)((uint64_t)block + sizeof(struct BlockDescriptor));

				/* get next block */
				block = (volatile struct BlockDescriptor* )((uint64_t)(block) + block->size);

				block->flags = KMEM_BLOCK_FREE;
				block->size = KMEM_BLOCK_SIZEMASK & (t0 - alen);
				
				kreleaseStaticMutex(((uint64_t*)heapbase)[2]);
				return ret;
			}

			/* otherwise overallocate entire block */
			else if (block->size >= alen){
				block->flags = KMEM_BLOCK_USED;
				ret = (void*)((uint64_t)block + sizeof(struct BlockDescriptor));

				kreleaseStaticMutex(((uint64_t*)heapbase)[2]);
				return ret;
			}
		}

		/* get next block */
		block = (volatile struct BlockDescriptor* )((uint64_t)(block) + block->size);
	}

	/* out of memory */
	
	/* attempt allocating memory */
	const uint64_t csize = heapbase[1].size;
	const uint64_t tsize = csize + alen + sizeof(struct BlockDescriptor) * 4; // add four for the reserved blocks
	if (tsize < heapbase[0].size) {
		while (heapbase[1].size < tsize) {
			heapbase[1].size *= 2;
		}

		if (kmmap(kPML4T, (void*)((uint64_t)heapbase + csize), KMEM_PAGE_PRESENT | KMEM_PAGE_WRITE, heapbase[1].size - csize) == 1) {
			return 0; /* out of pages */
		}
		kreleaseStaticMutex(((uint64_t*)heapbase)[2]);
		return _kmalloc(heapbase, length);
	}

	/* TODO: implement errno */
	kreleaseStaticMutex(((uint64_t*)heapbase)[2]);
	return 0; /* out of virtual address space */

}

void* kzalloc(uint64_t length) {
	void* ret = kmalloc(length);
	if (ret == 0) {
		return 0;
	}

	kfill(ret, length, 0);
	return ret;
}

void* kcalloc(uint64_t count, uint64_t length) {
	return kzalloc(count * length);
}

void* krealloc(void* ptr, uint64_t length) {
	volatile struct BlockDescriptor* block = (volatile struct BlockDescriptor* )((uint64_t)ptr - sizeof(struct BlockDescriptor));
	void* ret;

	//TODO: peek if next if free or last
	if (block->size >= length + sizeof(struct BlockDescriptor)) {
		return ptr;
	}
	else {
		ret = kmalloc(length);
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
	volatile struct BlockDescriptor* block = (volatile struct BlockDescriptor* )((uint64_t)ptr - sizeof(struct BlockDescriptor));
	volatile const struct BlockDescriptor* nblk = (volatile struct BlockDescriptor* )((uint64_t)(block) + block->size);

	/* check if next is last */
	if (nblk->flags == KMEM_BLOCK_LAST) {
		block->flags = KMEM_BLOCK_LAST;
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

#endif /* CORE_MEMORY_C */
