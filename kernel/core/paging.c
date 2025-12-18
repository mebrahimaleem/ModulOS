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
#include <core/mm_init.h>
#include <core/panic.h>

#include <lib/mem_utils.h>

#define PAGE_PS 			0x80
#define PAGE_TBL_FLG	(PAGE_PRESENT | PAGE_RW)

#define PAGE_SIZE	0x1000

#define TABLE_PADDR_MASK 	0xFFFFFFFFFFFFF000

#define KERNEL_VMA 	0xFFFFFFFF80000000

#define POOL_SIZE	0x200000

#define GET_TABLE(entry) ((uint64_t*)((uint64_t)entry & TABLE_PADDR_MASK))

#define PDPT_PADDR_MASK	0xFFFFFFFFC0000000
#define PD_PADDR_MASK		0xFFFFFFFFFFE00000

#define GET_PT_INDEX(addr) 		((addr & 0x1FF000) >> 12)
#define GET_PD_INDEX(addr) 		((addr & 0x3FE00000) >> 21)
#define GET_PDPT_INDEX(addr)	((addr & 0x7FC0000000) >> 30)
#define GET_PML4_INDEX(addr)	((addr & 0xFF8000000000) >> 39)

struct page_table_t;

extern uint64_t kernel_pml4[512];

static uint64_t bootstrap_pdpt[512] __attribute__((aligned(PAGE_SIZE)));
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

static struct paging_pool_header_t* early_create_pool(void);
static struct paging_pool_header_t* create_pool(void);

static uint64_t early_alloc_page(void) {
	if (hint->used == 512) {
		// last pool is always empty
		for (hint = root_pool; hint->used == 512; hint = hint->next);

		if (hint->ac == 0) {
			hint->ac = 1;
			hint->next = early_create_pool();
		}
	}

	for (; hint->hint < 64; hint->hint++) {
		if (hint->bitmap[hint->hint] != 0xFF) {
			for (uint8_t i = 0; i < 8; i++) {
				if (((hint->bitmap[hint->hint] & (1 << i))) != (1 << i)) {
					hint->bitmap[hint->hint] |= 1 << i;
					hint->used++;
					const uint64_t addr = (uint64_t)hint + PAGE_SIZE * ((uint64_t)hint->hint * 8 + (uint64_t)i);
					memset((void*)addr, 0, PAGE_SIZE);
					return addr;
				}
			}
		}
	}

	hint->hint = 0;
	return early_alloc_page();
}

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
				if (((hint->bitmap[hint->hint] & (1 << i))) != (1 << i)) {
					hint->bitmap[hint->hint] |= 1 << i;
					hint->used++;
					const uint64_t addr = (uint64_t)hint + PAGE_SIZE * ((uint64_t)hint->hint * 8 + (uint64_t)i);
					memset((void*)addr, 0, PAGE_SIZE);
					return addr;
				}
			}
		}
	}

	hint->hint = 0;
	return alloc_page();
}

static struct paging_pool_header_t* early_create_pool() {
	struct paging_pool_header_t* const addr = (struct paging_pool_header_t*)mm_early_alloc_2m();
	paging_early_map_2m((uint64_t)addr, (uint64_t)addr, PAGE_PRESENT | PAGE_RW);

	memset(addr, 0, POOL_SIZE);
	addr->bitmap[0] = 0x01;

	return addr;
}

static struct paging_pool_header_t* create_pool() {
	struct paging_pool_header_t* const addr = (struct paging_pool_header_t*)mm_alloc(MM_ORDER_2M);
	paging_map((uint64_t)addr, (uint64_t)addr, PAGE_PRESENT | PAGE_RW, PAGE_2M);

	memset(addr, 0, POOL_SIZE);
	addr->bitmap[0] = 0x01;

	return addr;
}

void paging_init() {
	const uint64_t pool_base = mm_early_alloc_2m();

	uint64_t pdpt = (uint64_t)kernel_pml4[GET_PML4_INDEX(pool_base)];

	if ((pdpt & PAGE_PRESENT) != PAGE_PRESENT) {
		pdpt = ((uint64_t)&bootstrap_pdpt[0] - KERNEL_VMA) | PAGE_PRESENT | PAGE_RW;
		kernel_pml4[GET_PML4_INDEX(pool_base)] = pdpt;
	}


	uint64_t pd = GET_TABLE(pdpt)[GET_PDPT_INDEX(pool_base)];

	if ((pd & PAGE_PS) != PAGE_PS) {
		GET_TABLE(pdpt)[GET_PDPT_INDEX(pool_base)] = ((uint64_t)&bootstrap_pd[0] - KERNEL_VMA) |
			PAGE_PRESENT | PAGE_RW;

		bootstrap_pd[GET_PD_INDEX(pool_base)] = pool_base | PAGE_PRESENT | PAGE_RW | PAGE_PS;
	}

	root_pool = (struct paging_pool_header_t*)pool_base;
	hint = root_pool;

	memset(root_pool, 0, POOL_SIZE);

	root_pool->ac = 1;
	root_pool->bitmap[0] = 0x01;
	root_pool->next = early_create_pool();
}

void paging_map(uint64_t vaddr, uint64_t paddr, uint8_t flg, enum page_size_t page_size) {
	uint64_t pdpt = (uint64_t)kernel_pml4[GET_PML4_INDEX(vaddr)];

	if ((pdpt & PAGE_PRESENT) != PAGE_PRESENT) {
		pdpt = alloc_page() | PAGE_TBL_FLG;
		kernel_pml4[GET_PML4_INDEX(vaddr)] = pdpt;
	}

	uint64_t pd = GET_TABLE(pdpt)[GET_PDPT_INDEX(vaddr)];

	if ((pd & PAGE_PRESENT) == PAGE_PRESENT && (pd & PAGE_PS) == PAGE_PS) {
		if ((pd & TABLE_PADDR_MASK) == (paddr & PDPT_PADDR_MASK)) {
			return;
		}

		panic(PANIC_PAGING);
	}

	if (page_size == PAGE_1G) {
		if ((pd & PAGE_PRESENT) == PAGE_PRESENT) {
			panic(PANIC_PAGING);
		}

		GET_TABLE(pdpt)[GET_PDPT_INDEX(vaddr)] = paddr | flg | PAGE_PS;
		return;
	}

	if ((pd & PAGE_PRESENT) != PAGE_PRESENT) {
		pd = alloc_page() | PAGE_TBL_FLG;
		GET_TABLE(pdpt)[GET_PDPT_INDEX(vaddr)] = pd;
	}

	uint64_t pt = GET_TABLE(pd)[GET_PD_INDEX(vaddr)];

	if ((pt & PAGE_PRESENT) == PAGE_PRESENT && (pt & PAGE_PS) == PAGE_PS) {
		if ((pt & TABLE_PADDR_MASK) == (paddr & PD_PADDR_MASK)) {
			return;
		}

		panic(PANIC_PAGING);
	}

	if (page_size == PAGE_2M) {
		if ((pt & PAGE_PRESENT) == PAGE_PRESENT) {
			panic(PANIC_PAGING);
		}

		GET_TABLE(pd)[GET_PD_INDEX(vaddr)] = paddr | flg | PAGE_PS;
		return;
	}

	if ((pt & PAGE_PRESENT) != PAGE_PRESENT) {
		pt = alloc_page() | PAGE_TBL_FLG;
		GET_TABLE(pd)[GET_PD_INDEX(vaddr)] = pt;
	}

	const uint64_t addr = GET_TABLE(pt)[GET_PT_INDEX(vaddr)];

	if ((addr & PAGE_PRESENT) == PAGE_PRESENT) {
		if ((addr & TABLE_PADDR_MASK) == paddr) {
			return;
		}

		panic(PANIC_PAGING);
	}

	GET_TABLE(pt)[GET_PT_INDEX(vaddr)] = paddr | flg;
}

void paging_early_map_2m(uint64_t vaddr, uint64_t paddr, uint8_t flg) {
	uint64_t pdpt = (uint64_t)kernel_pml4[GET_PML4_INDEX(vaddr)];

	if ((pdpt & PAGE_PRESENT) != PAGE_PRESENT) {
		pdpt = early_alloc_page() | PAGE_TBL_FLG;
		kernel_pml4[GET_PML4_INDEX(vaddr)] = pdpt;
	}

	uint64_t pd = GET_TABLE(pdpt)[GET_PDPT_INDEX(vaddr)];

	if ((pd & PAGE_PS) == PAGE_PS) {
		if ((pd & TABLE_PADDR_MASK) == (paddr & PDPT_PADDR_MASK)) {
			return;
		}

		panic(PANIC_PAGING);
	}

	if ((pd & PAGE_PRESENT) != PAGE_PRESENT) {
		pd = early_alloc_page() | PAGE_TBL_FLG;
		GET_TABLE(pdpt)[GET_PDPT_INDEX(vaddr)] = pd;
	}

	const uint64_t pt = GET_TABLE(pd)[GET_PD_INDEX(vaddr)];

	if ((pt & PAGE_PRESENT) == PAGE_PRESENT) {
		if ((pt & TABLE_PADDR_MASK) == (paddr & PD_PADDR_MASK)) {
			return;
		}

		panic(PANIC_PAGING);
	}

	GET_TABLE(pd)[GET_PD_INDEX(vaddr)] = paddr | flg | PAGE_PS;
}
