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
#include <core/logging.h>
#include <core/panic.h>

#define HEADER_SIZE_MASK	0xFFFFFFFFFFFFFFF8uLL
#define HEADER_USED				0x1uLL

struct free_node_t {
	uint64_t base;
	size_t size;
	struct free_node_t* next;
};

struct alloc_header_t {
	uint64_t header;
	uint64_t resv;
} __attribute__((packed));

_Static_assert(sizeof(struct alloc_header_t) == 16, "struct alloc_header_t must be 16 bytes wide");

static struct free_node_t* free_list;
static struct free_node_t* node_pool;
static uint8_t alloc_lock;

void alloc_init(void) {
	lock_init(&alloc_lock);
	free_list = 0;
	node_pool = 0;
}

static struct free_node_t* alloc_node(void) {
	struct free_node_t* ret = 0;
	uint64_t addr, i;

	lock_acquire(&alloc_lock);
	if (node_pool) {
		ret = node_pool;
		node_pool = node_pool->next;
	}
	lock_release(&alloc_lock);

	if (!ret) {
		addr = mm_alloc_p(PAGE_SIZE_4K);
		if (!addr) {
			logging_log_error("Failed to allocate free list pointers for heap");
			panic(PANIC_NO_MEM);
		}

		addr = paging_ident(addr);
		ret = (struct free_node_t*)addr;
		for (i = 1; i < PAGE_SIZE_4K / sizeof(struct free_node_t) - 1; i++) {
			ret[i].next = &ret[i+1];
		}

		lock_acquire(&alloc_lock);
		ret[PAGE_SIZE_4K / sizeof(struct free_node_t) - 1].next = node_pool;
		node_pool = &ret[1];
		lock_release(&alloc_lock);
	}

	ret->next = 0;
	return ret;
}

void* kmalloc(size_t size) {
	struct free_node_t* node, **next;
	uint64_t ret = 0, adj;
	
	size += sizeof(struct alloc_header_t);

	adj = size % 16;
	if (adj) {
		size += 16 - adj;
	}

	lock_acquire(&alloc_lock);
	node = free_list;
	next = &free_list;

	while (node) {
		if (node->size >= size) {
			ret = node->base;
			node->size -= size;
			node->base += size;

			if (!node->size) {
				*next = node->next;
				node->next = node_pool;
				node_pool = node;
			}

			break;
		}

		next = &node->next;
		node = node->next;
	}

	lock_release(&alloc_lock);

	if (!ret) {
		adj = size % PAGE_SIZE_4K;
		if (adj) {
			size += PAGE_SIZE_4K - adj;
		}

		ret = mm_alloc_p(size); // aligness does not matter for ident
		if (!ret) {
			logging_log_error("Failed to allocate memory for heap");
			return 0;
		}

		ret = paging_ident(ret);
	}

	((struct alloc_header_t*)ret)->header = (size) | HEADER_USED;
	ret += sizeof(struct alloc_header_t);

	return (void*)ret;
}

void kfree(void* ptr) {
	if (!ptr) {
		return;
	}

	struct alloc_header_t* header = (struct alloc_header_t*)((uint64_t)ptr - sizeof(struct alloc_header_t));
	if (!(header->header & HEADER_USED)) {
		logging_log_warning("Double free @ 0x%lx", ptr);
		return;
	}

	header->header &= ~HEADER_USED;

	struct free_node_t* node = alloc_node();
	node->size = header->header & HEADER_SIZE_MASK;
	node->base = (uint64_t)header;

	lock_acquire(&alloc_lock);
	node->next = free_list;
	free_list = node;
	lock_release(&alloc_lock);
}
