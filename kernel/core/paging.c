/* paging.c - paging managger */
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

#include <core/paging.h>
#include <core/alloc.h>
#include <core/mm.h>
#include <core/panic.h>
#include <core/logging.h>
#include <core/lock.h>
#include <core/cpu_instr.h>

#include <lib/kmemset.h>

#include <drivers/apic/ipi.h>

#define PAGE_PS 			0x80

#define PAGE_ADDR_MASK				0x0000FFFFFFFFF000
#define PAGE_ADDR_PAT_MASK		0x0000FFFFFFFFE000

#define GET_PT_INDEX(addr) 		((addr & 0x1FF000) >> 12)
#define GET_PD_INDEX(addr) 		((addr & 0x3FE00000) >> 21)
#define GET_PDPT_INDEX(addr)	((addr & 0x7FC0000000) >> 30)
#define GET_PML4_INDEX(addr)	((addr & 0xFF8000000000) >> 39)

#define AVL_GUARD	0x800

extern uint64_t kernel_pml4[512];

static uint8_t paging_lock;

static enum page_size_t page_walk(uint64_t vaddr, uint64_t** access) {
	uint64_t entry;
	*access = (uint64_t*)paging_ident((uint64_t)&kernel_pml4[0]);

	entry = (*access)[GET_PML4_INDEX(vaddr)];
	*access = &(*access)[GET_PML4_INDEX(vaddr)];
	if (!(entry & PAGE_PRESENT)) {
		return _PAGE_512G;
	}
	*access = (uint64_t*)paging_ident((**access & PAGE_ADDR_MASK));

	entry = (*access)[GET_PDPT_INDEX(vaddr)];
	*access = &(*access)[GET_PDPT_INDEX(vaddr)];
	if (!(entry & PAGE_PRESENT)) {
		return PAGE_1G;
	}

	if (entry & PAGE_PS) {
		return PAGE_1G;
	}
	*access = (uint64_t*)paging_ident((**access & PAGE_ADDR_MASK));

	entry = (*access)[GET_PD_INDEX(vaddr)];
	*access = &(*access)[GET_PD_INDEX(vaddr)];
	if (!(entry & PAGE_PRESENT)) {
		return PAGE_2M;
	}

	if (entry & PAGE_PS) {
		return PAGE_2M;
	}
	*access = (uint64_t*)paging_ident((**access & PAGE_ADDR_MASK));

	*access = &(*access)[GET_PT_INDEX(vaddr)];
	return PAGE_4K;
}

static uint64_t* increase_granularity(uint64_t vaddr, uint64_t* access, enum page_size_t lvl, enum page_size_t page_size) {
	for (; lvl > page_size; lvl--) {
		*access = mm_alloc_p(0x1000);
		if (!access) {
			return 0;
		}
		*access |= PAGE_PRESENT | PAGE_RW;
		access = (uint64_t*)paging_ident((*access & PAGE_ADDR_MASK));
		kmemset(access, 0, 0x1000);

		switch (lvl) {
			case _PAGE_512G:
				access = &(access)[GET_PDPT_INDEX(vaddr)];
				break;
			case PAGE_1G:
				access = &(access)[GET_PD_INDEX(vaddr)];
				break;
			default:
				access = &(access)[GET_PT_INDEX(vaddr)];
				break;
		}
	}

	return access;
}

void paging_init(void) {
	lock_init(&paging_lock);
}

uint64_t paging_map(uint64_t vaddr, uint64_t paddr, uint16_t flg, enum page_size_t page_size) {
	uint64_t* access;
	enum page_size_t lvl = page_walk(vaddr, &access);

	if (lvl < page_size) {
		logging_log_error("Cannot override page of finer granularity from 0x%lx-0x%lx (%u) to 0x%lx-0x%lx (%u)",
				vaddr, *access - IDENT_BASE, (uint32_t)lvl, vaddr, paddr | flg, (uint32_t)page_size);
		return *access - IDENT_BASE;
	}

	if (*access & PAGE_PRESENT) {
		logging_log_error("Cannot override page from 0x%lx-0x%lx (%u) to 0x%lx-0x%lx (%u)",
				vaddr, *access, (uint32_t)lvl, vaddr, paddr | flg, (uint32_t)page_size);
		return *access;
	}

	access = increase_granularity(vaddr, access, lvl, page_size);
	if (!access) {
		return 0;
	}

	if (page_size != PAGE_4K) {
		flg |= PAGE_PS;
	}

	*access = paddr | flg;
	return paddr;
}

void paging_unmap(uint64_t vaddr, enum page_size_t page_size) {
	uint64_t* access;
	enum page_size_t lvl = page_walk(vaddr, &access);

	if (lvl != page_size) {
		logging_log_error("Cannot unmap page of different granularity");
		return;
	}


	*access = 0;
	apic_tlb_shootdown(vaddr);
}

uint64_t paging_ident(uint64_t paddr) {
	return paddr + IDENT_BASE;
}

void paging_install_guard(uint64_t vaddr) {
	uint64_t* access;
	lock_acquire(&paging_lock);
	enum page_size_t lvl = page_walk(vaddr, &access);

	if (*access & PAGE_PRESENT) {
		logging_log_error("Cannot install page guard over mapped page");
		panic(PANIC_STATE);
	}

	access = increase_granularity(vaddr, access, lvl, PAGE_4K);
	if (!access) {
		logging_log_error("Failed to install page guard");
		panic(PANIC_STATE);
	}

	*access |= AVL_GUARD;
	lock_release(&paging_lock);
}

void paging_remove_guard(uint64_t vaddr) {
	uint64_t* access;
	lock_acquire(&paging_lock);
	enum page_size_t lvl = page_walk(vaddr, &access);

	if (lvl != PAGE_4K || !(*access & AVL_GUARD)) {
		logging_log_error("Attempted to remove guard from ungaurded page @ 0x%x", vaddr);
	}	

	*access = 0;
	lock_release(&paging_lock);
}

uint8_t paging_check_guard(uint64_t vaddr) {
	uint64_t* access, access_value;
	lock_acquire(&paging_lock);
	enum page_size_t lvl = page_walk(vaddr, &access);

	access_value = *access;
	lock_release(&paging_lock);

	return lvl == PAGE_4K && (access_value & AVL_GUARD);
}
