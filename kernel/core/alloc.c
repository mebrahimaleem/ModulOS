/* alloc.c - heap allocator */
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

#include <stdint.h>
#include <stddef.h>

#include <core/alloc.h>
#include <core/lock.h>
#include <core/mm.h>
#include <core/paging.h>
#include <core/proc_data.h>

#define SLAB_SIZE_1			0x1
#define SLAB_SIZE_2			0x2
#define SLAB_SIZE_4			0x4
#define SLAB_SIZE_8			0x8
#define SLAB_SIZE_16		0x10
#define SLAB_SIZE_32		0x20
#define SLAB_SIZE_64		0x40
#define SLAB_SIZE_128		0x80
#define SLAB_SIZE_256		0x100
#define SLAB_SIZE_512		0x200
#define SLAB_SIZE_1K		0x400
#define SLAB_SIZE_2K		0x800
#define SLAB_SIZE_4K		0x1000
#define SLAB_SIZE_8K		0x2000
#define SLAB_SIZE_16K		0x4000
#define SLAB_SIZE_32K		0x8000
#define SLAB_SIZE_64K		0x10000
#define SLAB_SIZE_128K	0x20000
#define SLAB_SIZE_256K	0x40000
#define SLAB_SIZE_512K	0x80000
#define SLAB_SIZE_1M		0x100000
#define SLAB_SIZE_2M		0x200000

struct cache_node_t {
	uint64_t base;
	struct cache_node_t* next;
};

static struct cache_node_t* create_node_block(struct cache_node_t** tail) {
	uint64_t i;

	struct cache_node_t* list = (struct cache_node_t*)paging_ident(mm_alloc_p2m());

	for (i = 0; i < SIZE_2M / sizeof(struct cache_node_t) - 1; i++) {
		list[i].next = &list[i+1];
		*tail = &list[i+1];
	}

	return list;
}

static struct cache_node_t* alloc_cache_node(void) {
	struct proc_data_t* data = proc_data_get();
	struct cache_node_t* t;

	lock_acquire(&data->alloc_caches.lock);
	t = data->alloc_caches.avl;
	if (t) {
		data->alloc_caches.avl = t->next;
	}
	lock_release(&data->alloc_caches.lock);

	if (t) {
		return t;
	}

	struct cache_node_t* tail;
	t = create_node_block(&tail);
	if (t) {
		lock_acquire(&data->alloc_caches.lock);
		tail->next = data->alloc_caches.avl;
		data->alloc_caches.avl = t->next;
		lock_release(&data->alloc_caches.lock);
	}

	return t;
}

static enum slab_t min_slab(size_t size) {
	if (size <= SLAB_SIZE_1) {
		return SLAB_1;
	}
	if (size <= SLAB_SIZE_2) {
		return SLAB_2;
	}
	if (size <= SLAB_SIZE_4) {
		return SLAB_4;
	}
	if (size <= SLAB_SIZE_8) {
		return SLAB_8;
	}
	if (size <= SLAB_SIZE_16) {
		return SLAB_16;
	}
	if (size <= SLAB_SIZE_32) {
		return SLAB_32;
	}
	if (size <= SLAB_SIZE_64) {
		return SLAB_64;
	}
	if (size <= SLAB_SIZE_128) {
		return SLAB_128;
	}
	if (size <= SLAB_SIZE_256) {
		return SLAB_256;
	}
	if (size <= SLAB_SIZE_512) {
		return SLAB_512;
	}
	if (size <= SLAB_SIZE_1K) {
		return SLAB_1K;
	}
	if (size <= SLAB_SIZE_2K) {
		return SLAB_2K;
	}
	if (size <= SLAB_SIZE_4K) {
		return SLAB_4K;
	}
	if (size <= SLAB_SIZE_8K) {
		return SLAB_8K;
	}
	if (size <= SLAB_SIZE_16K) {
		return SLAB_16K;
	}
	if (size <= SLAB_SIZE_32K) {
		return SLAB_32K;
	}
	if (size <= SLAB_SIZE_64K) {
		return SLAB_64K;
	}
	if (size <= SLAB_SIZE_128K) {
		return SLAB_128K;
	}
	if (size <= SLAB_SIZE_256K) {
		return SLAB_256K;
	}
	if (size <= SLAB_SIZE_512K) {
		return SLAB_512K;
	}
	if (size <= SLAB_SIZE_1M) {
		return SLAB_1M;
	}
	if (size <= SLAB_SIZE_2M) {
		return SLAB_2M;
	}
	return SLAB_MAX;
}

static struct cache_node_t* create_cache(enum slab_t slab, struct cache_node_t** tail) {
	uint64_t i;

	uint64_t slab_base = paging_ident(mm_alloc_p2m());

	struct cache_node_t* cache = 0, * t;

	for (i = 0; i < SIZE_2M; i += (1 << (uint64_t)slab)) {
		t = alloc_cache_node();
		if (!t) {
			break;
		}
		t->next = cache;
		t->base = i + slab_base;
		if (!cache) {
			*tail = t;
		}
		cache = t;
	}

	return cache;
}

void alloc_init(void) {
	struct proc_data_t* data = proc_data_get();
	uint64_t i;

	data->alloc_caches.avl = 0;
	lock_init(&data->alloc_caches.lock);

	for (i = 0; i < SLAB_MAX; i++) {
		data->alloc_caches.slabs[i] = 0;
	}
}

void* kmalloc(size_t size) {
	enum slab_t slab = min_slab(size);
	if (slab == SLAB_MAX) {
		return 0;
	}

	uint64_t ret;
	struct proc_data_t* data = proc_data_get();
	lock_acquire(&data->alloc_caches.lock);
	struct cache_node_t* cache = data->alloc_caches.slabs[slab], * tail;
	if (cache) {
		ret = cache->base;
		data->alloc_caches.slabs[slab] = cache->next;

		//TODO: track cache for freeing
		lock_release(&data->alloc_caches.lock);
	}
	else {
		lock_release(&data->alloc_caches.lock);
		cache = create_cache(slab, &tail);
		if (!cache) {
			return 0;
		}

		lock_acquire(&data->alloc_caches.lock);
		tail->next = data->alloc_caches.slabs[slab];
		data->alloc_caches.slabs[slab] = cache->next;

		ret = cache->base;

		//TODO: track cache for freeing
		lock_release(&data->alloc_caches.lock);
	}

	return (void*)ret;
}

void kfree(void* ptr) {
	(void)ptr;
}
