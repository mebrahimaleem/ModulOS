/* paging.c - paging managger */
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

#include <stdint.h>
#include <stddef.h>

#include <core/paging.h>
#include <core/mm.h>
#include <core/panic.h>

#include <lib/mem_utils.h>

#define PAGE_PS 			0x80

#define PAGE_SIZE	0x1000

#define TABLE_PADDR_MASK 	0xFFFFFFFFFFFFF000

#define KERNEL_VMA 	0xFFFFFFFF80000000

#define POOL_SIZE	0x200000

#define GET_TABLE(entry) ((uint64_t*)((uint64_t)entry & TABLE_PADDR_MASK))

#define GET_PT_INDEX(addr) 		((addr & 0x1FF000) >> 12)
#define GET_PD_INDEX(addr) 		((addr & 0x3FE00000) >> 21)
#define GET_PDPT_INDEX(addr)	((addr & 0x7FC0000000) >> 30)
#define GET_PML4_INDEX(addr)	((addr & 0xFF8000000000) >> 39)

struct page_table_t;

extern uint64_t kernel_pml4[512];

static uint64_t bootstrap_pd[512] __attribute__((aligned(PAGE_SIZE)));

struct paging_pool_header_t {
	uint16_t used;
	struct paging_pool_header_t* next;
	uint8_t hint;
	uint8_t ac;
	uint8_t bitmap[64];
	uint8_t resv[4020];
} __attribute__((packed));

_Static_assert(sizeof(struct paging_pool_header_t) == PAGE_SIZE);

struct paging_pool_header_t* root_pool;

struct paging_pool_header_t* hint;

static struct paging_pool_header_t* create_pool(void);

static uint64_t alloc_page(void) {
	if (hint->used == 512) {
		// last pool is always empty
		for (hint = root_pool; hint->used == 512; hint = hint->next);

		if (hint->ac == 0) {
			hint->ac = 1;
			hint->next = create_pool();
		}
	}

	for (; hint->hint < 64; hint->hint++) {
		if (hint->bitmap[hint->hint] != 0xFF) {
			for (uint8_t i = 0; i < 8; i++) {
				if ((hint->bitmap[hint->hint] & (1 >> i)) != (1 >> i)) {
					hint->bitmap[hint->hint] |= 1 >> i;
					hint->used++;
					return (uint64_t)hint + PAGE_SIZE * ((uint64_t)hint->hint * 8 + (uint64_t)i) - KERNEL_VMA;
				}
			}
		}
	}

	hint->hint = 0;
	return alloc_page();

	panic(PANIC_PAGING);
	return 0;
}

static struct paging_pool_header_t* create_pool() {
	struct paging_pool_header_t* const addr = (struct paging_pool_header_t*)mm_alloc_pv(POOL_SIZE);
	paging_map_2m((uint64_t)addr, mm_alloc_frame_cont(POOL_SIZE / PAGE_SIZE, POOL_SIZE / PAGE_SIZE),
			PAGE_PRESENT | PAGE_RW);

	memset(addr, 0, POOL_SIZE);
	addr->bitmap[0] = 0x01;

	return addr;
}

void paging_init(uint64_t paging_base) {
	const uint64_t pool_base = mm_alloc_pv(POOL_SIZE);

	// only support in last pdpt
	if (GET_PML4_INDEX(pool_base) != 511) {
		panic(PANIC_PAGING);
	}

	GET_TABLE(kernel_pml4
		[511])
		[GET_PDPT_INDEX(pool_base)] =
			((uint64_t)&bootstrap_pd[0] - KERNEL_VMA) | PAGE_PRESENT | PAGE_RW;

	GET_TABLE(GET_TABLE(kernel_pml4
		[511])
		[GET_PDPT_INDEX(pool_base)])
		[GET_PD_INDEX(pool_base)] =
			paging_base | PAGE_PRESENT | PAGE_RW | PAGE_PS;

	root_pool = (struct paging_pool_header_t*)pool_base;
	hint = root_pool;

	memset(root_pool, 0, POOL_SIZE);

	root_pool->ac = 1;
	root_pool->bitmap[0] = 0x01;
}

void paging_init_post() {
	root_pool->next = create_pool();
}

void paging_map_2m(uint64_t vaddr, uint64_t paddr, uint8_t flg) {
	uint64_t pdpt = (uint64_t)kernel_pml4[GET_PML4_INDEX(vaddr)];

	if ((pdpt & PAGE_PRESENT) != PAGE_PRESENT) {
		pdpt = alloc_page() | flg;
		kernel_pml4[GET_PML4_INDEX(vaddr)] = pdpt;
	}

	uint64_t pd = GET_TABLE(pdpt)[GET_PDPT_INDEX(vaddr)];

	if ((pd & PAGE_PS) == PAGE_PS) {
		panic(PANIC_PAGING);
	}

	if ((pd & PAGE_PRESENT) != PAGE_PRESENT) {
		pd = alloc_page() | flg;
		GET_TABLE(pdpt)[GET_PDPT_INDEX(vaddr)] = alloc_page() | flg;
	}

	const uint64_t pt = GET_TABLE(pd)[GET_PD_INDEX(vaddr)];

	if ((pt & PAGE_PRESENT) == PAGE_PRESENT) {
		panic(PANIC_PAGING);
	}


	GET_TABLE(pd)[GET_PD_INDEX(vaddr)] = paddr | flg | PAGE_PS;
}
