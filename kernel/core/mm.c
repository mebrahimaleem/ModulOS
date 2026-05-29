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
#include <core/cpu_instr.h>
#include <core/process.h>
#include <core/scheduler.h>
#include <core/time.h>
#include <core/signal.h>

#include <apic/ipi.h>

#define PAGE_4K_MASK		0xFFFFFFFFFFFFF000
#define SIZE_GIB				(1024 * 1024 * 1024)

#define MAX_INIT_NODES	64
#define WITHIN_NODE(base, b, l)	(base >= b && base < b + l)

#define SHOOTDOWN_DELAY_MS	5000

struct mm_list_node_t {
	struct mm_list_node_t* next;
	struct mm_list_node_t* prev;
	uint64_t base;
	size_t size;
};

struct disarm_list_t {
	struct disarm_list_t* next;
	uint8_t id;
	uint8_t state;
};

#ifdef DEBUG_LOGGING
uint64_t bytes_allocated;
uint64_t prev_bytes_allocated;
#endif /* DEBUG_LOGGING */

struct mm_list_node_t init_nodes[MAX_INIT_NODES];

extern uint8_t _kernel_pend;

static uint64_t kernel_limit;

struct mm_list_node_t* p_begin;
struct mm_list_node_t* p_end;

struct mm_list_node_t* v_begin;
struct mm_list_node_t* v_end;

struct mm_list_node_t v_init;

struct mm_list_node_t p_begin_guard;
struct mm_list_node_t p_end_guard;

struct mm_list_node_t v_begin_guard;
struct mm_list_node_t v_end_guard;

uint8_t p_lock;
uint8_t v_lock;
uint8_t f_lock;

struct mm_list_node_t* reuse_nodes;

static struct free_transaction_list_t* pending_free;
static struct free_transaction_list_t* transaction_list;
static uint8_t pending_free_lock;
static struct disarm_list_t* disarm_list;

static struct signal_wait_t* free_pending_wait;

static void free_node(struct mm_list_node_t* node) {
	lock_acquire(&f_lock);
	node->next = reuse_nodes;
	reuse_nodes = node;
	lock_release(&f_lock);
}

static struct mm_list_node_t* alloc_node(void) {
	struct mm_list_node_t* node;

	lock_acquire(&f_lock);
	node = reuse_nodes;
	if (reuse_nodes) {
		reuse_nodes = reuse_nodes->next;
	}
	lock_release(&f_lock);

	if (node) {
		return node;
	}

	return kmalloc(sizeof(struct mm_list_node_t));
}

static void insert_node(struct mm_list_node_t* node, struct mm_list_node_t* begin) {
	struct mm_list_node_t* insert;

	for (insert = begin->next; insert->base && insert->base < node->base; insert = insert->next);

	node->next = insert;
	node->prev = insert->prev;

	insert->prev->next = node;
	insert->prev = node;

	if (node->prev->base + node->prev->size == node->base) {
		node->prev->size += node->size;

		insert = node;

		node->prev->next = node->next;
		node->next->prev = node->prev;

		node = node->prev;

		free_node(insert);
	}

	if (node->base + node->size == node->next->base) {
		node->size += node->next->size;

		insert = node->next;

		node->next->next->prev = node;
		node->next = node->next->next;

		free_node(insert);
	}
}

void mm_init(
		void (*first_segment)(uint64_t* handle),
		void (*next_segment)(uint64_t* handle, struct mem_segment_t* seg)) {
	kernel_limit = (uint64_t)&_kernel_pend;

	uint64_t blocks = 0;
	uint64_t init_node_i = 0;

#ifdef DEBUG_LOGGING
	bytes_allocated = 0;
	prev_bytes_allocated = 0;
#endif /* DEBUG_LOGGING */

	lock_init(&p_lock);
	lock_init(&v_lock);
	lock_init(&f_lock);

	reuse_nodes = 0;

	// find memory limit
	uint64_t mem_limit = 0;

	uint64_t handle;
	struct mem_segment_t seg;

	uint64_t adj;

	p_begin_guard.base = 0;
	p_begin_guard.size = 0;
	p_begin_guard.next = &p_end_guard;

	p_end_guard.base = 0;
	p_end_guard.size = 0;
	p_end_guard.prev = &p_begin_guard;

	p_begin = &p_begin_guard;
	p_end = &p_end_guard;

	struct mm_list_node_t* temp;

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

		if (init_node_i == MAX_INIT_NODES) {
			logging_log_warning("Out of initial memory node memory. Skipping allocation of 0x%lx bytes", seg.size);
			continue;
		}

		temp = &init_nodes[init_node_i++];
		temp->base = seg.base;
		temp->size = seg.size;

		insert_node(temp, p_begin);
	}

	logging_log_info("Detected 0x%lX bytes (0x%lX GiB) of memory across %ld blocks",
			mem_limit, (uint64_t)(mem_limit / SIZE_GIB), blocks);

	v_init.base = CANON_HIGH;
	v_init.size = VIRTUAL_LIMIT - CANON_HIGH;
	v_init.prev = &v_begin_guard;
	v_init.next = &v_end_guard;

	v_begin_guard.base = 0;
	v_begin_guard.size = 0;
	v_begin_guard.next = &v_init;

	v_end_guard.base = 0;
	v_end_guard.size = 0;
	v_end_guard.prev = &v_init;

	v_begin = &v_begin_guard;
	v_end = &v_end_guard;

	logging_log_debug("Virtual space 0x%lx-0x%lx",
			v_init.base, v_init.base + v_init.size);

	logging_log_debug("Initializing heap allocator");
	alloc_init();
	logging_log_debug("Heap allocator init done");

	logging_log_debug("Initializing paging");
	paging_init();
	logging_log_debug("Paging init done");
}

static uint64_t mm_alloc_max(size_t size,
														 uint64_t align,
														 uint64_t max,
														 struct mm_list_node_t* end,
														 uint8_t* lock) {
	struct mm_list_node_t* node;

	struct mm_list_node_t* extra = 0;

	if (align) {
		extra = alloc_node();
	}

	if (!size) {
		return 0;
	}

	if (align % PAGE_SIZE_4K) {
		return 0;
	}

	uint64_t rem = size % PAGE_SIZE_4K;

	if (rem) {
		size += PAGE_SIZE_4K - rem;
	}

	const uint64_t asize = size;

	size += align;

	lock_acquire(lock);
	for (node = end->prev; node->base; node = node->prev) {
		if (node->base + node->size < max && node->size >= size) {
			break;
		}
	}

	if (!node->base) {
		lock_release(lock);
		return 0;
	}

	uint64_t adj = align ? node->base % align : 0;

	if (adj) {
		extra->base = node->base;
		extra->size = adj;

		node->base += adj;
		node->size -= adj;

		extra->prev = node->prev;
		extra->next = node;

		node->prev->next = extra;
		node->prev = extra;

		extra = 0;
	}

	node->size -= asize;
	uint64_t ret = node->base;

	if (node->size) {
		node->base += asize;
		node = 0;
	}
	else {
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}

	lock_release(lock);

	if (node) {
		free_node(node);
	}

	if (extra) {
		free_node(extra);
	}

#ifdef DEBUG_LOGGING
	bytes_allocated += asize;
#endif /* DEBUG_LOGGING */
	return ret;
}

static void mm_free(uint64_t base, uint64_t size, struct mm_list_node_t* begin, uint8_t* lock) {
	struct mm_list_node_t* insert = alloc_node();

	uint64_t rem = size % PAGE_SIZE_4K;

	if (rem) {
		size += PAGE_SIZE_4K - rem;
	}

#ifdef DEBUG_LOGGING
	bytes_allocated -= size;
#endif /* DEBUG_LOGGING */

	insert->base = base;
	insert->size = size;

	lock_acquire(lock);
	insert_node(insert, begin);
	lock_release(lock);
}

uint64_t mm_alloc_p(size_t size) {
	return mm_alloc_max(size, 0, ~0uLL, p_end, &p_lock);
}

uint64_t mm_alloc_v(size_t size) {
	return mm_alloc_max(size, 0, ~0uLL, v_end, &v_lock);
}

uint64_t mm_alloc_palign(size_t size, uint64_t align) {
	return mm_alloc_max(size, align, ~0uLL, p_end, &p_lock);
}

uint64_t mm_alloc_valign(size_t size, uint64_t align) {
	return mm_alloc_max(size, align, ~0uLL, v_end, &v_lock);
}

uint64_t mm_alloc_pmax(size_t size, uint64_t align, uint64_t max) {
	return mm_alloc_max(size, align, max, p_end, &p_lock);
}

uint64_t mm_alloc_vmax(size_t size, uint64_t align, uint64_t max) {
	return mm_alloc_max(size, align, max, v_end, &v_lock);
}

void mm_free_p(uint64_t base, size_t size) {
	mm_free(base, size, p_begin, &p_lock);
}

void mm_free_v(uint64_t base, size_t size) {
	struct free_transaction_list_t* pending = kmalloc(sizeof(struct free_transaction_list_t));

	pending->base = base;
	pending->size = size;
	lock_acquire(&pending_free_lock);
	pending->next = pending_free;
	pending_free = (struct free_transaction_list_t*)pending;
	lock_release(&pending_free_lock);

	if (free_pending_wait) {
		signal_awake(free_pending_wait);
	}
}

__attribute((noreturn)) static void free_all_pending(void* _ign) {
	(void)_ign;

	struct free_transaction_list_t* next;
	struct disarm_list_t* disarm;
	uint8_t cntrl;

	free_pending_wait = signal_wait_alloc();

	while (1) {
		time_sleep(SHOOTDOWN_DELAY_MS);

		while (!pending_free) {
			signal_wait(free_pending_wait);
		}

		lock_acquire(&pending_free_lock);

		if (!pending_free) {
			lock_release(&pending_free_lock);
			continue;
		}

		transaction_list = pending_free;
		pending_free = 0;

		for (disarm = disarm_list; disarm; disarm = disarm->next) {
			disarm->state = 1;
			apic_shootdown(disarm->id);
		}

		lock_release(&pending_free_lock);

		do {
			cntrl = 0;
			lock_acquire(&pending_free_lock);
			for (disarm = disarm_list; disarm; disarm = disarm->next) {
				if (disarm->state) {
					cntrl = 1;
					break;
				}
			}
			lock_release(&pending_free_lock);

			if (cntrl) {
				signal_wait(free_pending_wait);
			}
		} while(cntrl);

		while (transaction_list) {
			next = transaction_list->next;

			mm_free(transaction_list->base, transaction_list->size, v_begin, &v_lock);

			kfree(transaction_list);
			transaction_list = next;
		}
	}
}

void mm_transaction_init(void) {
	scheduler_schedule(process_from_func(free_all_pending, 0));
}

struct free_transaction_list_t* mm_get_shootdown_list(void) {
	return transaction_list;
}

void mm_register_barrier(uint8_t id) {
	struct disarm_list_t* l = kmalloc(sizeof(struct disarm_list_t));
	l->id = id;
	l->state = 0;

	lock_acquire(&pending_free_lock);
	l->next = disarm_list;
	disarm_list = l;
	lock_release(&pending_free_lock);
}

void mm_barrier_disarm(uint8_t id) {
	for (struct disarm_list_t* l = disarm_list; l; l = l->next) {
		if (id == l->id) {
			l->state = 0;
			break;
		}
	}

	signal_awake(free_pending_wait);
}

#ifdef DEBUG_LOGGING
void mm_log_usage(void) {
	const char* s = "";

	if (bytes_allocated > prev_bytes_allocated) {
		s = "+";
	}

	logging_log_debug("MM - Bytes Allocated %lu (%s%ld)",
			bytes_allocated, s, bytes_allocated - prev_bytes_allocated);

	prev_bytes_allocated = bytes_allocated;
}
#endif /* DEBUG_LOGGING */
