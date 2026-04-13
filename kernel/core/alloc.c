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
#include <core/cpu_instr.h>

#define TYPE_HEADER_USED	0x0uLL
#define TYPE_HEADER_FREE	0x1uLL
#define TYPE_HEADER_LAST	0x2uLL

#define MASK_HEADER_USED	0x1uLL
#define MASK_HEADER_LAST	0x2uLL

#define ARENA_SIZE				0x200000
#define INIT_BLOCK_SIZE		(ARENA_SIZE - sizeof(struct alloc_arena_t))

#define ALLOC_ALIGN				16

#define IS_USED(x)				((x & MASK_HEADER_USED) == TYPE_HEADER_USED)
#define IS_FREE(x)				((x & MASK_HEADER_USED) == TYPE_HEADER_FREE)
#define IS_LAST(x)				((x & MASK_HEADER_LAST) == TYPE_HEADER_LAST)

#define GET_SIZE(x)				(x & ~0xFuLL)
#define GET_FLAGS(x)			(x & 0xFuLL);

_Static_assert(GET_SIZE(ALLOC_ALIGN) == ALLOC_ALIGN, "Bad size mask");

struct alloc_arena_t;

struct alloc_header_t {
	uint64_t size;
	struct alloc_header_t* prev;
	union {
		struct alloc_header_t* next_free;
		struct alloc_arena_t* arena;
	} next_free;
	struct alloc_header_t* prev_free;
};

struct alloc_arena_t {
	struct alloc_header_t* free;
	struct alloc_arena_t* next;
	uint64_t resv;
	uint8_t arena_lock;
};

_Static_assert(sizeof(struct alloc_header_t) % ALLOC_ALIGN == 0, "Bad alloc header struct");
_Static_assert(sizeof(struct alloc_arena_t) % ALLOC_ALIGN == 0, "Bad arena metadata struct");

static struct alloc_arena_t* arena_head;
static uint8_t alloc_lock;

static inline struct alloc_header_t* get_next(struct alloc_header_t* header) {
	return IS_LAST(header->size) ? 0 : (struct alloc_header_t*)((uint64_t)header + GET_SIZE(header->size));
}

void alloc_init() {
	lock_init(&alloc_lock);

	arena_head = 0;
}

static void patch_list(struct alloc_arena_t* arena, struct alloc_header_t* header) {
	if (header->prev_free) {
		header->prev_free->next_free.next_free = header->next_free.next_free;

		if (header->next_free.next_free) {
			header->next_free.next_free->prev_free = header->prev_free;
		}
	}
	else {
		arena->free = header->next_free.next_free;

		if (header->next_free.next_free) {
			header->next_free.next_free->prev_free = 0;
		}
	}
}

static void* alloc(struct alloc_arena_t* arena, struct alloc_header_t* header, size_t size) {
	struct alloc_header_t* split_header;

	if (IS_USED(header->size)) {
		logging_log_error("Inconsistent heap state. Attempted allocation on used block");
		panic(PANIC_STATE);
	}

	if (size > GET_SIZE(header->size)) {
		return 0;
	}

	// split block
	if (GET_SIZE(header->size) - size > sizeof(struct alloc_header_t)) {
		split_header = (struct alloc_header_t*)((uint64_t)header + size);
		split_header->prev = header;

		split_header->size = (GET_SIZE(header->size) - size) | TYPE_HEADER_FREE;

		if (IS_LAST(header->size)) {
			split_header->size |= TYPE_HEADER_LAST;
			header->size &= ~TYPE_HEADER_LAST;
		}
		else {
			get_next(header)->prev = split_header;
		}

		// patch free list
		split_header->next_free.next_free = header->next_free.next_free;
		split_header->prev_free = header;

		if (header->next_free.next_free) {
			header->next_free.next_free->prev_free = split_header;
		}

		header->next_free.next_free = split_header;

		header->size = size; // general case deals with flags
	}

	header->size = GET_SIZE(header->size) | TYPE_HEADER_USED | (header->size & MASK_HEADER_LAST);

	// patch free list
	patch_list(arena, header);

	header->next_free.arena = arena;

	return (void*)((uint64_t)header + sizeof(struct alloc_header_t));
}

void* kmalloc(size_t size) {
	struct alloc_arena_t* i;
	struct alloc_header_t* header;
	void* ret;
	uint64_t arena_base;

	size_t adjusted_size = size + sizeof(struct alloc_header_t);
	if (adjusted_size % ALLOC_ALIGN) {
		adjusted_size += ALLOC_ALIGN - (adjusted_size % ALLOC_ALIGN);
	}

	lock_acquire(&alloc_lock);
	i = arena_head;
	lock_release(&alloc_lock);

	while (i) {
		lock_acquire(&i->arena_lock);	

		if (!i->free) {
			goto next_arena;
		}
		
		for (header = i->free; header; header = header->next_free.next_free) {
			if (GET_SIZE(header->size) >= adjusted_size) {
				ret = alloc(i, header, adjusted_size);
				lock_release(&i->arena_lock);
				return ret;
			}
		}

next_arena:
		lock_release(&i->arena_lock);

		lock_acquire(&alloc_lock);
		i = i->next;
		lock_release(&alloc_lock);
	};

	// out of heap space, new arena
	arena_base = mm_alloc_p(ARENA_SIZE);

	if (!arena_base) {
		logging_log_error("Out of memory for heap");
		return 0;
	}

	arena_base = paging_ident(arena_base);

	i = (struct alloc_arena_t*)arena_base;
	i->free = (struct  alloc_header_t*)(arena_base + sizeof(struct alloc_arena_t));
	lock_init(&i->arena_lock);

	*i->free = (struct alloc_header_t) {
		.size = INIT_BLOCK_SIZE | TYPE_HEADER_FREE | TYPE_HEADER_LAST,
		.prev = 0,
		.next_free.next_free = 0,
		.prev_free = 0
	};

	ret = alloc(i, i->free, adjusted_size);

	lock_acquire(&alloc_lock);
	i->next = arena_head;
	arena_head = i;
	lock_release(&alloc_lock);

	return ret;
}

void kfree(void* ptr) {
	if (!ptr) {
		return;
	}

	struct alloc_header_t* header = (struct alloc_header_t*)((uint64_t)ptr - sizeof(struct alloc_header_t));
	struct alloc_arena_t* arena;
	struct alloc_header_t* next;

	if (IS_FREE(header->size)) {
		logging_log_warning("Double free @ 0x%x", ptr);
		return;
	}

	arena = header->next_free.arena;

	lock_acquire(&arena->arena_lock);
	lock_release(&arena->arena_lock);

	header->size = GET_SIZE(header->size) | TYPE_HEADER_FREE | (header->size & MASK_HEADER_LAST);

	// coallecse with prev
	if (header->prev && IS_FREE(header->prev->size)) {
		header->prev->size = (GET_SIZE(header->prev->size) + GET_SIZE(header->size)) | TYPE_HEADER_FREE | (header->size & MASK_HEADER_LAST);

		header = header->prev;
	}
	else {
		// insert into free list's begining
		header->next_free.next_free = arena->free;
		header->prev_free = 0;

		if (arena->free) {
			arena->free->prev_free = header;
		}

		arena->free = header;
	}

	// coallecse with next
	next = get_next(header);
	if (!IS_LAST(header->size) && IS_FREE(next->size)) {
		header->size = (GET_SIZE(header->size) + GET_SIZE(next->size)) | TYPE_HEADER_FREE | (next->size & MASK_HEADER_LAST);

		patch_list(arena, next);
	}



	lock_release(&arena->arena_lock);
}
