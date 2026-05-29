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
#include <lib/kmemcpy.h>

#include <apic/ipi.h>

#define PAGE_PS 			0x80

#define PAGE_ADDR_MASK				0x0000FFFFFFFFF000uLL
#define PAGE_ADDR_PAT_MASK		0x0000FFFFFFFFE000uLL

#define GET_PT_INDEX(addr) 		((addr & 0x1FF000) >> 12)
#define GET_PD_INDEX(addr) 		((addr & 0x3FE00000) >> 21)
#define GET_PDPT_INDEX(addr)	((addr & 0x7FC0000000) >> 30)
#define GET_PML4_INDEX(addr)	((addr & 0xFF8000000000) >> 39)

#define AVL_GUARD	0x800

#define PML4_CONSISTENT_START	256
#define PML4_CONSISTENT_END		512

#define PAGE_KEEP_FLAGS	(PAGE_PRESENT | PAGE_RW | PAGE_US | PAGE_XD | PAGE_PS)

extern uint64_t kernel_pml4[512];

static uint8_t paging_lock;

static enum page_size_t page_walk(uint64_t vaddr, uint64_t** access, uint64_t* pml4) {
	uint64_t entry;
	*access = (uint64_t*)paging_ident((uint64_t)pml4);

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
		*access = mm_alloc_p(PAGE_SIZE_4K);
		if (!access) {
			return 0;
		}
		*access |= PAGE_PRESENT | PAGE_RW | PAGE_US;
		access = (uint64_t*)paging_ident((*access & PAGE_ADDR_MASK));
		kmemset(access, 0, PAGE_SIZE_4K);

		switch (lvl) {
			case _PAGE_512G:
				access = &(access)[GET_PDPT_INDEX(vaddr)];
				break;
			case PAGE_1G:
				access = &(access)[GET_PD_INDEX(vaddr)];
				break;
			case PAGE_2M:
				access = &(access)[GET_PT_INDEX(vaddr)];
				break;
			case PAGE_4K:
				logging_log_error("Invalid call to increase granularity");
				panic(PANIC_STATE);
		}
	}

	return access;
}

void paging_init(void) {
	lock_init(&paging_lock);
}

void paging_ensure_mapped(void) {
	uint64_t addr;

	for (uint16_t i = PML4_CONSISTENT_START; 	i < PML4_CONSISTENT_END; i++) {
		if (kernel_pml4[i] & PAGE_PRESENT) {
			continue;
		}

		addr = mm_alloc_p(PAGE_SIZE_4K);

		if (!addr) {
			logging_log_error("Failed to map consistency page");
			panic(PANIC_NO_MEM);
		}

		kernel_pml4[i] = addr | PAGE_PRESENT | PAGE_RW;

		addr = paging_ident(addr);
		kmemset((void*)addr, 0, PAGE_SIZE_4K);
	}
}

uint64_t paging_map_proc(uint64_t vaddr, uint64_t paddr, uint64_t flg, enum page_size_t page_size, uint64_t* pml4) {
	uint64_t* access;
	lock_acquire(&paging_lock);
	enum page_size_t lvl = page_walk(vaddr, &access, pml4);

	if (lvl < page_size) {
		lock_release(&paging_lock);
		logging_log_error("Cannot override page of finer granularity from 0x%lx-0x%lx (%u) to 0x%lx-0x%lx (%u)",
				vaddr, *access - IDENT_BASE, (uint32_t)lvl, vaddr, paddr | flg, (uint32_t)page_size);
		return *access - IDENT_BASE;
	}

	if (*access & PAGE_PRESENT) {
		lock_release(&paging_lock);
		logging_log_error("Cannot override page from 0x%lx-0x%lx (%u) to 0x%lx-0x%lx (%u)",
				vaddr, *access, (uint32_t)lvl, vaddr, paddr | flg, (uint32_t)page_size);
		return *access;
	}

	access = increase_granularity(vaddr, access, lvl, page_size);
	if (!access) {
		lock_release(&paging_lock);
		return 0;
	}

	if (page_size != PAGE_4K) {
		flg |= PAGE_PS;
	}

	*access = paddr | flg;
	lock_release(&paging_lock);
	return paddr;
}

uint64_t paging_map(uint64_t vaddr, uint64_t paddr, uint64_t flg, enum page_size_t page_size) {
	return paging_map_proc(vaddr, paddr, flg, page_size, kernel_pml4);
}

uint8_t paging_update_perms(uint64_t vaddr, uint64_t flg, enum page_size_t page_size, uint64_t* pml4) {
	uint64_t* access;

	lock_acquire(&paging_lock);
	enum page_size_t lvl = page_walk(vaddr, &access, pml4);

	if (lvl != page_size) {
		lock_release(&paging_lock);
		logging_log_error("Cannot update page perms of different granularity");
		return 1;
	}

	*access &= ~(PAGE_PRESENT | PAGE_RW | PAGE_US | PAGE_XD);
	*access |= flg;
	lock_release(&paging_lock);

	return 0;
}

void paging_unmap(uint64_t vaddr, enum page_size_t page_size) {
	uint64_t* access;
	lock_acquire(&paging_lock);
	enum page_size_t lvl = page_walk(vaddr, &access, kernel_pml4);

	if (lvl != page_size) {
		lock_release(&paging_lock);
		logging_log_error("Cannot unmap page of different granularity");
		return;
	}


	*access = 0;
	lock_release(&paging_lock);
}

uint64_t paging_ident(uint64_t paddr) {
	return paddr + IDENT_BASE;
}

void paging_install_guard(uint64_t vaddr) {
	uint64_t* access;
	lock_acquire(&paging_lock);
	enum page_size_t lvl = page_walk(vaddr, &access, kernel_pml4);

	if (*access & PAGE_PRESENT) {
		logging_log_error("Cannot install page guard over mapped page");
		goto cleanup;
	}

	access = increase_granularity(vaddr, access, lvl, PAGE_4K);
	if (!access) {
		logging_log_error("Failed to install page guard");
		goto cleanup;
	}

	*access |= AVL_GUARD;

cleanup:
	lock_release(&paging_lock);
}

void paging_remove_guard(uint64_t vaddr) {
	uint64_t* access;
	lock_acquire(&paging_lock);
	enum page_size_t lvl = page_walk(vaddr, &access, kernel_pml4);

	if (lvl != PAGE_4K || !(*access & AVL_GUARD)) {
		logging_log_error("Attempted to remove guard from ungaurded page @ 0x%x", vaddr);
	}	
	else {
		*access = 0;
	}

	lock_release(&paging_lock);
}

uint8_t paging_check_guard(uint64_t vaddr) {
	uint64_t* access, access_value;
	lock_acquire(&paging_lock);
	enum page_size_t lvl = page_walk(vaddr, &access, kernel_pml4);

	access_value = *access;
	lock_release(&paging_lock);

	return lvl == PAGE_4K && (access_value & AVL_GUARD);
}

uint64_t paging_create_pml4(void) {
	uint64_t* access;
	uint64_t* k_access;
	uint64_t pml4 = mm_alloc_p(PAGE_SIZE_4K);
	uint16_t i;

	if (!pml4) {
		return 0;
	}

	access = (uint64_t*)paging_ident(pml4);
	k_access = (uint64_t*)paging_ident((uint64_t)&kernel_pml4[0]);

	kmemset(access, 0, PAGE_SIZE_4K);

	// copy upper half top level pages
	for (i = PML4_CONSISTENT_START; i < PML4_CONSISTENT_END; i++) {
		access[i] = k_access[i];
	}

	return pml4;
}

static void free_pages(uint64_t entry, enum page_size_t lvl) {
	uint64_t* access = (uint64_t*)paging_ident((entry & PAGE_ADDR_MASK));

	for (uint16_t i = 0; i < 512; i++) {
		if (access[i] & PAGE_PRESENT) {
			if (lvl != PAGE_4K && !(access[i] & PAGE_PS)) {
				free_pages(access[i], lvl-1);
			}
		}
	}

	mm_free_p(entry & PAGE_ADDR_MASK, PAGE_SIZE_4K);
}

void paging_free_userspace(uint64_t* pml4) {
	uint64_t* access = (uint64_t*)paging_ident((uint64_t)pml4);

	for (uint16_t i = 0; i < PML4_CONSISTENT_START; i++) {
		if (access[i] & PAGE_PRESENT) {
			free_pages(access[i], PAGE_1G);
		}
	}

	mm_free_p((uint64_t)pml4, PAGE_SIZE_4K);
}

static void copy_pages(uint64_t access_, uint64_t copy_, enum page_size_t lvl) {
	uint64_t* access = (uint64_t*)paging_ident(access_ & PAGE_ADDR_MASK);
	uint64_t* copy = (uint64_t*)paging_ident(copy_ & PAGE_ADDR_MASK);

	kmemset((void*)copy, 0, PAGE_SIZE_4K);

	for (uint16_t i = 0; i < 512; i++) {
		if (access[i] & PAGE_PRESENT) {
			copy[i] = mm_alloc_p(PAGE_SIZE_4K);
			copy[i] |= access[i] & PAGE_KEEP_FLAGS;

			if (lvl != PAGE_4K && !(access[i] & PAGE_PS)) {
				copy_pages(access[i], copy[i], lvl - 1);
			}
			else {
				void* d = (void*)paging_ident(copy[i] & PAGE_ADDR_MASK);
				void* s = (void*)paging_ident(access[i] & PAGE_ADDR_MASK);

				switch (lvl) {
					case PAGE_4K:
						kmemcpy(d, s, PAGE_SIZE_4K);
						break;
					case PAGE_2M:
						kmemcpy(d, s, PAGE_SIZE_2M);
						break;
					case PAGE_1G:
						kmemcpy(d, s, PAGE_SIZE_1G);
						break;
					case _PAGE_512G:
						break;
				}
			}
		}
	}
}

uint64_t paging_duplicate_lower(uint64_t cr3) {
	uint64_t new_cr3 = paging_create_pml4();

	uint64_t* access = (uint64_t*)paging_ident((uint64_t)cr3);
	uint64_t* copy = (uint64_t*)paging_ident((uint64_t)new_cr3);

	for (uint16_t i = 0; i < PML4_CONSISTENT_START; i++) {
		if (access[i] & PAGE_PRESENT) {
			copy[i] = mm_alloc_p(PAGE_SIZE_4K);
			copy[i] |= access[i] & PAGE_KEEP_FLAGS;

			copy_pages(access[i], copy[i], PAGE_1G);
		}
	}

	return new_cr3;
}
