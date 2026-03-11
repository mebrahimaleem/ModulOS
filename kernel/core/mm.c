/* mm.c - memory manager */
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

#include <core/mm.h>
#include <core/paging.h>
#include <core/alloc.h>
#include <core/lock.h>
#include <core/logging.h>
#include <core/panic.h>

#include <lib/mergesort.h>

#define PAGE_4K_MASK		0xFFFFFFFFFFFFF000
#define SIZE_GIB				(1024 * 1024 * 1024)

#define MAX_INIT_NODES	64
#define WITHIN_NODE(base, b, l)	(base >= b && base < b + l)

struct mm_tree_node_t {
	struct mm_tree_node_t* less;
	struct mm_tree_node_t* more;
	uint64_t base;
	uint64_t limit;
};

static struct mm_tree_node_t* p_tree;
static struct mm_tree_node_t* v_tree;

extern uint8_t _kernel_pend;

static uint64_t kernel_limit;

static uint8_t p_lock;
static uint8_t v_lock;
static uint8_t n_lock;

static struct mm_tree_node_t node_pool[MAX_INIT_NODES];
static struct mm_tree_node_t* free_nodes;

static struct mm_tree_node_t* alloc_node(void) {
	struct mm_tree_node_t* next;
	lock_acquire(&n_lock);
	if (free_nodes) {
		next = free_nodes;
		free_nodes = free_nodes->less;
		lock_release(&n_lock);
	}
	else {
		lock_release(&n_lock);
		next = kmalloc(sizeof(struct mm_tree_node_t));
	}

	next->less = 0;
	next->more = 0;
	return next;
}

static void free_node(struct mm_tree_node_t* node) {
	lock_acquire(&n_lock);
	node->less = free_nodes;
	free_nodes = node;
	lock_release(&n_lock);
}

static struct mm_tree_node_t* find_base_node(uint64_t base, struct mm_tree_node_t* root, struct mm_tree_node_t* parent) {
	while (1) {
		if (!root) {
			return parent;
		}

		if (WITHIN_NODE(base, root->base, root->limit)) {
			return root;
		}

		parent = root;

		if (base < root->base) {
			root = root->less;
		}
		else {
			root = root->more;
		}
	}
}

static void attach_node(struct mm_tree_node_t* root, struct mm_tree_node_t* node) {
	if (node->base > root->base) {
		if (root->more) {
			attach_node(root->more, node);
		}
		else {
			root->more = node;
		}
	}
	else {
		if (root->less) {
			attach_node(root->less, node);
		}
		else {
			root->less = node;
		}
	}
}

static uint64_t mm_alloc(
		uint64_t limit,
		struct mm_tree_node_t* root,
		uint64_t max,
		struct mm_tree_node_t* parent) {
	uint64_t ret;

	if (!root) {
		return 0;
	}

	if (root->base + limit > max) {
		return mm_alloc(limit, root->less, max, root);
	}

	ret = mm_alloc(limit, root->more, max, root);
	if (ret) {
		return ret;
	}

	if (limit <= root->limit) {
		root->limit -= limit;
		ret = root->base;
		root->base += limit;

		if (root->limit == 0) {
			if (root == parent->less) {
				parent->less = 0;
			}
			else {
				parent->more = 0;
			}

			if (root->less) {
				attach_node(parent, root->less);
			}
			if (root->more) {
				attach_node(parent, root->more);
			}

			free_node(root);
		}

		return ret;
	}

	return mm_alloc(limit, root->less, max, root);
}

static void mm_free(uint64_t base, uint64_t size, struct mm_tree_node_t* root, uint8_t* lock) {
	uint64_t adj;
	struct mm_tree_node_t* node;

	adj = size % PAGE_SIZE_4K;
	if (adj) {
		size += PAGE_SIZE_4K - adj;
	}

	lock_acquire(lock);

	node = find_base_node(base, root->more, root);

	if (WITHIN_NODE(base, node->base, node->limit)) {
		if (root == p_tree) {
			logging_log_warning("Double free @ 0x%lx on p_tree", base);
		}
		else {
			logging_log_warning("Double free @ 0x%lx on v_tree", base);
		}
	}
	else if (base < node->base) {
		node->less = alloc_node();
		node->less->base = base;
		node->less->limit = size;
	}
	else {
		node->more = alloc_node();
		node->more->base = base;
		node->more->limit = size;
	}

	//TODO: coallese

	lock_release(lock);
}

static uint64_t mm_alloc_max(size_t size, uint64_t align, uint64_t max, struct mm_tree_node_t* root, uint8_t* lock) {
	uint64_t ret;
	uint64_t adj;
	struct mm_tree_node_t* padding;

	adj = size % PAGE_SIZE_4K;
	if (adj) {
		size += PAGE_SIZE_4K - adj;
	}

	adj = align % PAGE_SIZE_4K;
	if (adj) {
		align += PAGE_SIZE_4K - adj;
	}

	lock_acquire(lock);
	ret = mm_alloc(size + align, root->more, max, root);
	lock_release(lock);

	if (align) {
		adj = ret % align;
		if (adj) {
			adj = align - adj;

			padding = alloc_node();
			padding->base = ret;
			padding->limit = adj;

			lock_acquire(lock);
			attach_node(root, padding);
			lock_release(lock);
		}
	}

	return ret + adj;
}

extern void mm_init(
		void (*first_segment)(uint64_t* handle),
		void (*next_segment)(uint64_t* handle, struct mem_segment_t* seg)) {
	kernel_limit = (uint64_t)&_kernel_pend;

	lock_init(&p_lock);
	lock_init(&v_lock);
	lock_init(&n_lock);

	uint64_t blocks;

	for (blocks = 0; blocks < MAX_INIT_NODES - 1; blocks++) {
		node_pool[blocks].less = &node_pool[blocks + 1];	
	}
	node_pool[MAX_INIT_NODES - 1].less = 0;
	free_nodes = &node_pool[0];

	
	// find memory limit
	uint64_t mem_limit = 0;
	blocks = 0;


	uint64_t handle;
	struct mem_segment_t seg;

	struct mm_tree_node_t* node;
	uint64_t adj;

	p_tree = alloc_node();
	p_tree->base = 0;
	p_tree->limit = 0;
	p_tree->less = 0;
	p_tree->more = 0;

	first_segment(&handle);
	for (next_segment(&handle, &seg); seg.size || seg.base; next_segment(&handle, &seg)) {
		logging_log_debug("Memory segment 0x%lX (base) 0x%lX (size) 0x%lX (type)",
				(uint64_t)seg.base, (uint64_t)seg.size, (uint64_t)seg.type);
		if (seg.type == MEM_AVL && seg.base + seg.size > mem_limit) {
			mem_limit = seg.base + seg.size;
			blocks++;
		}

		if (seg.type != MEM_AVL) {
			continue;
		}
	
		while (seg.size > PAGE_SIZE_4K && seg.base < kernel_limit) {
			seg.base += PAGE_SIZE_4K;
			seg.size -= PAGE_SIZE_4K;
		}

		if (seg.size < PAGE_SIZE_4K || seg.base < kernel_limit) {
			continue;
		}

		// align base
		adj = seg.base % PAGE_SIZE_4K;
		if (adj) {
			adj = PAGE_SIZE_4K - adj;
			seg.base += adj;
			seg.size -= adj;
		}

		if (seg.size < PAGE_SIZE_4K) {
			continue;
		}

		// trim limit
		adj = seg.size % PAGE_SIZE_4K;
		if (adj) {
			seg.size -= adj;
		}

		if (seg.size < PAGE_SIZE_4K) {
			continue;
		}

		node = find_base_node(seg.base, p_tree->more, p_tree);	
		if (WITHIN_NODE(seg.base, node->base, node->limit)) {
			logging_log_error("Overlapping memory regions 0x%lx into 0x%lx-0x%lx",
					seg.base, node->base, node->base + node->limit);
			panic(PANIC_STATE);
		}

		if (seg.base < node->base) {
			node->less = alloc_node();
			node = node->less;
		}
		else {
			node->more = alloc_node();
			node = node->more;
		}

		node->base = seg.base;
		node->limit = seg.size;
	}

	logging_log_info("Detected 0x%lX bytes (0x%lX GiB) of memory across %ld blocks",
			mem_limit, (uint64_t)(mem_limit / SIZE_GIB), blocks);

	v_tree = alloc_node();
	v_tree->base = 0;
	v_tree->limit = 0;
	v_tree->less = 0;
	v_tree->more = alloc_node();

	v_tree->more->base = CANON_HIGH;
	v_tree->more->limit = VIRTUAL_LIMIT - CANON_HIGH + 1;
	v_tree->more->less = 0;
	v_tree->more->more = 0;

	logging_log_debug("Virtual space 0x%lx-0x%lx",
			v_tree->more->base, v_tree->more->base + v_tree->more->limit);

	logging_log_debug("Initializing heap allocator");
	alloc_init();
	logging_log_debug("Heap allocator init done");

	logging_log_debug("Initializing paging");
	paging_init();
	logging_log_debug("Paging init done");
}

uint64_t mm_alloc_p(size_t size) {
	return mm_alloc_max(size, 0, ~0uLL, p_tree, &p_lock);
}

uint64_t mm_alloc_v(size_t size) {
	return mm_alloc_max(size, 0, ~0uLL, v_tree, &v_lock);
}

uint64_t mm_alloc_palign(size_t size, uint64_t align) {
	return mm_alloc_max(size, align, ~0uLL, p_tree, &p_lock);
}

uint64_t mm_alloc_valign(size_t size, uint64_t align) {
	return mm_alloc_max(size, align, ~0uLL, v_tree, &v_lock);
}

uint64_t mm_alloc_pmax(size_t size, uint64_t align, uint64_t max) {
	return mm_alloc_max(size, align, max, p_tree, &p_lock);
}

uint64_t mm_alloc_vmax(size_t size, uint64_t align, uint64_t max) {
	return mm_alloc_max(size, align, max, v_tree, &v_lock);
}

void mm_free_p(uint64_t base, size_t size) {
	mm_free(base, size, p_tree, &p_lock);
}

void mm_free_v(uint64_t base, size_t size) {
	mm_free(base, size, v_tree, &v_lock);
}
