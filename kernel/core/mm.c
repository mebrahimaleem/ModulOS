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

#include <lib/mergesort.h>

#define PAGE_4K_MASK		0xFFFFFFFFFFFFF000
#define SIZE_GIB				(1024 * 1024 * 1024)

#define MAX_BLOCKS	64

struct frame_list_t {
	uint64_t base;
	uint64_t limit;
	struct frame_list_t* next;
};

extern uint8_t _kernel_pend;

static void (*early_first_segment)(uint64_t* handle);
static void (*early_next_segment)(uint64_t* handle, struct mem_segment_t* seg);
static uint64_t kernel_limit;

static struct frame_list_t init_v_frame;

static struct frame_list_t* p_list;
static struct frame_list_t* v_list;

static struct frame_list_t* p_2m_list;
static struct frame_list_t* v_2m_list;

static uint8_t p_lock;
static uint8_t v_lock;

static uint8_t p_2m_lock;
static uint8_t v_2m_lock;

static struct frame_list_t init_p_array[3 * MAX_BLOCKS];

static uint64_t mm_alloc(size_t size, struct frame_list_t** list, void** tofree) {
	struct frame_list_t* prev = 0;
	uint64_t ret;
	*tofree = 0;

	for (struct frame_list_t* i = *list; i != 0; i = i->next) {
		if (i->limit == size) {
			if (prev) {
				prev->next = i->next;
				ret = i->base;
				*tofree = i;
				return ret;
			}

			ret = i->base;
			*list = i->next;
			return ret;
		}
		if (i->limit > size) {
			ret = i->base;
			i->base += size;
			i->limit -= size;
			return ret;
		}
	}

	logging_log_error("Out of contigious memory");
	return 0;
}

void mm_init(
		void (*first_segment)(uint64_t* handle),
		void (*next_segment)(uint64_t* handle, struct mem_segment_t* seg)) {
	
	kernel_limit = (uint64_t)&_kernel_pend;
	early_first_segment = first_segment;
	early_next_segment = next_segment;

	// initial vmem is always 2m aligned
	v_2m_list = &init_v_frame;
	init_v_frame.base = CANON_HIGH;
	init_v_frame.limit = VIRTUAL_LIMIT - CANON_HIGH + 1;

	v_list = 0;
	p_list = 0;
	p_2m_list = 0;

	lock_init(&p_lock);
	lock_init(&v_lock);

	lock_init(&p_2m_lock);
	lock_init(&v_2m_lock);
	
	// find memory limit
	uint64_t mem_limit = 0;
	uint64_t blocks = 0;

	uint64_t handle;
	struct mem_segment_t seg;

	first_segment(&handle);
	for (early_next_segment(&handle, &seg); seg.size || seg.base; next_segment(&handle, &seg)) {
		logging_log_debug("Memory segment 0x%lX (base) 0x%lX (size) 0x%lX (type)",
				(uint64_t)seg.base, (uint64_t)seg.size, (uint64_t)seg.type);
		if (seg.type == MEM_AVL && seg.base + seg.size > mem_limit) {
			mem_limit = seg.base + seg.size;
			blocks++;
		}
	}

	logging_log_info("Detected 0x%lX bytes (0x%lX GiB) of memory across %ld blocks",
			mem_limit, (uint64_t)(mem_limit / SIZE_GIB), blocks);

	logging_log_debug("Initializing heap allocator");
	alloc_init();
	logging_log_debug("Heap allocator init done");

	logging_log_debug("Initializing paging");
	paging_init();
	logging_log_debug("Paging init done");

	struct frame_list_t* init_p = &init_p_array[0];

	uint64_t skip = 0;
	if (blocks > MAX_BLOCKS) {
		skip = blocks - MAX_BLOCKS; // prefer later blocks as they are larger
	}
	uint64_t rem;
	first_segment(&handle);
	for (early_next_segment(&handle, &seg); seg.size || seg.base; ) {
		if (seg.type != MEM_AVL || seg.size < PAGE_SIZE_4K || skip) {
			goto next_seg;
		}

		if (skip) {
			skip--;
		}

		if (seg.base <= kernel_limit) {
			seg.base += PAGE_SIZE_4K;
			seg.size -= PAGE_SIZE_4K;
			continue;
		}

		// align base to 4K
		rem = seg.base % PAGE_SIZE_4K;

		if (rem) {
			seg.base += PAGE_SIZE_4K - rem;
			seg.size -= PAGE_SIZE_4K - rem;
		}

		// try 2m first
		if (seg.size >= 2 * SIZE_2M) {
			// pre padding
			rem = seg.base % SIZE_2M ;
			uint64_t base = seg.base;
			if (rem) {
				init_p->base = seg.base;
				init_p->limit = SIZE_2M - rem;
				init_p->next = p_list;
				p_list = init_p++;

				base += SIZE_2M - rem;
			}

			// post padding
			rem = seg.size % SIZE_2M ;
			uint64_t limit = seg.size;
			if (rem > PAGE_SIZE_4K) {
				init_p->base = seg.base + seg.size - rem;
				init_p->limit = rem & PAGE_4K_MASK;
				init_p->next = p_list;
				p_list = init_p++;

				limit -= rem & PAGE_4K_MASK;
			}

			init_p->base = base;
			init_p->limit = limit;
			init_p->next = p_2m_list;
			p_2m_list = init_p++;
			goto next_seg;
		}

		// otherwise add 4K
		init_p->base = seg.base;
		init_p->limit = seg.size & PAGE_4K_MASK;
		init_p->next = p_list;
		p_list = init_p++;

next_seg:
		next_segment(&handle, &seg);
	}
}

uint64_t mm_alloc_p(size_t size) {
	if (size % PAGE_SIZE_4K) {
		size += PAGE_SIZE_4K - (size % PAGE_SIZE_4K);
	}

	void* tofree;
	lock_acquire(&p_lock);
	if (!p_list) {
		lock_release(&p_lock);
		lock_acquire(&p_2m_lock);
		if (!p_2m_list) {
			logging_log_error("Out of physical memory");
			return 0;
		}
		struct frame_list_t* temp = p_2m_list;
		p_2m_list = p_2m_list->next;
		lock_release(&p_2m_lock);
		lock_acquire(&p_lock);
		temp->next = p_list;
		p_list = temp;
	}
	const uint64_t ret = mm_alloc(size, &p_list, &tofree);
	lock_release(&p_lock);
	if (tofree) {
		kfree(tofree);
	}

	return ret;
}

uint64_t mm_alloc_v(size_t size) {
	if (size % PAGE_SIZE_4K) {
		size += PAGE_SIZE_4K - (size % PAGE_SIZE_4K);
	}

	void* tofree;
	lock_acquire(&v_lock);
	if (!v_list) {
		lock_release(&v_lock);
		lock_acquire(&v_2m_lock);
		if (!v_2m_list) {
			logging_log_error("Out of virtual memory");
			return 0;
		}
		struct frame_list_t* temp = v_2m_list;
		v_2m_list = v_2m_list->next;
		lock_release(&v_2m_lock);
		lock_acquire(&v_lock);
		temp->next = v_list;
		v_list = temp;
	}
	const uint64_t ret = mm_alloc(size, &v_list, &tofree);
	lock_release(&v_lock);
	if (tofree) {
		kfree(tofree);
	}

	return ret;
}

uint64_t mm_alloc_p2m(void) {
	void* tofree;
	lock_acquire(&p_2m_lock);
	const uint64_t ret = mm_alloc(SIZE_2M , &p_2m_list, &tofree);
	lock_release(&p_2m_lock);

	if (tofree) {
		kfree(tofree);
	}

	return ret;
}

uint64_t mm_alloc_v2m(void) {
	void* tofree;
	lock_acquire(&v_2m_lock);
	const uint64_t ret = mm_alloc(SIZE_2M , &v_2m_list, &tofree);
	lock_release(&v_2m_lock);

	if (tofree) {
		kfree(tofree);
	}

	return ret;
}

void mm_free_p(uint64_t addr, size_t size) {
	struct frame_list_t* prepend = kmalloc(sizeof(struct frame_list_t));
	prepend->base = addr;
	prepend->limit = size;
	lock_acquire(&p_lock);
	prepend->next = p_list;
	p_list = prepend;
	lock_release(&p_lock);
}

void mm_free_v(uint64_t addr, size_t size) {
	struct frame_list_t* prepend = kmalloc(sizeof(struct frame_list_t));
	prepend->base = addr;
	prepend->limit = size;
	lock_acquire(&v_lock);
	prepend->next = v_list;
	v_list = prepend;
	lock_release(&v_lock);
}

void mm_free_p2m(uint64_t addr) {
	struct frame_list_t* prepend = kmalloc(sizeof(struct frame_list_t));
	prepend->base = addr;
	prepend->limit = SIZE_2M ;
	lock_acquire(&p_2m_lock);
	prepend->next = p_2m_list;
	p_2m_list = prepend;
	lock_release(&p_2m_lock);
}

void mm_free_v2m(uint64_t addr) {
	struct frame_list_t* prepend = kmalloc(sizeof(struct frame_list_t));
	prepend->base = addr;
	prepend->limit = SIZE_2M ;
	lock_acquire(&v_2m_lock);
	prepend->next = v_2m_list;
	v_2m_list = prepend;
	lock_release(&v_2m_lock);
}

static void** sort_next(void* _node) {
	struct frame_list_t* node = _node;
	return (void**)&node->next;
}

static uint64_t sort_value(void* _node) {
	struct frame_list_t* node = _node;
	return node->base;
}

static void defrag(
		struct frame_list_t** list,
		struct frame_list_t** m_list,
		uint8_t* lock,
		uint8_t* m_lock,
		struct frame_list_t** tofree) {

	struct frame_list_t* promote = 0;
	struct frame_list_t* i, * t, *prev = 0;
	
	lock_acquire(m_lock);
	lock_acquire(lock);	
	*list = mergesort_ll_inplace_ul(*list, sort_next, sort_value);

	for (i = *list; i && i->next; ) {
		if (i->base + i->limit == i->next->base) {
			t = i->next;	
			i->next = t->next;
			i->limit += t->limit;

			if (i->limit >= SIZE_2M && i->base % SIZE_2M == 0) {
				t->base = i->base;
				t->limit = i->limit - (i->limit % SIZE_2M);
				t->next = promote;
				promote = t;

				i->limit -= t->limit;
				if (i->limit) {
					i->base += t->limit;
				}
				else if (prev){
					prev->next = i->next;
					i->next = *tofree;
					*tofree = i;
					i = prev->next;
					continue;
				}
				else {
					*list = (*list)->next;
					i->next = *tofree;
					*tofree = i;
					i = *list;
					continue;
				}
			}
			else {
				t->next = *tofree;
				*tofree = t;
			}
			continue;
		}

		prev = i;
		i = i->next;
	}

	for (; promote; promote = t) {
		t = promote->next;
		promote->next = *m_list;
		*m_list = promote;
	}

	*m_list = mergesort_ll_inplace_ul(*m_list, sort_next, sort_value);
	for (i = *m_list; i && i->next; ) {
		if (i->base + i->limit == i->next->base) {
			t = i->next;	
			i->next = t->next;
			i->limit += t->limit;

			t->next = *tofree;
			*tofree = t;
			continue;
		}

		i = i->next;
	}
	lock_release(lock);
	lock_release(m_lock);
}

void mm_defrag(void) {
	struct frame_list_t* tofree = 0, * t;

	defrag(&p_list, &p_2m_list, &p_lock, &p_2m_lock, &tofree);
	defrag(&v_list, &v_2m_list, &v_lock, &v_2m_lock, &tofree);

	for (; tofree; tofree = t) {
		t = tofree->next;
		kfree(tofree);
	}
}
