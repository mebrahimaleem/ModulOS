/* apic_init.h - Advanced programmable interrupt controller initialization */
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


#include <apic/apic_init.h>

#include <kernel/core/logging.h>
#include <kernel/core/msr.h>
#include <kernel/core/alloc.h>
#include <kernel/core/paging.h>
#include <kernel/core/mm.h>

#define APIC_BASE_MASK	0xFFFFFFFFFF000
#define APIC_AE					0x800

#define OFF_APIC_ID		0x0020

struct apic_t {
	uint64_t v_base;
};

static struct apic_t* apics;

void apic_init(void) {
	struct apic_t* bs_apic = kmalloc(sizeof(struct apic_t));
	bs_apic->v_base = mm_alloc_dv(MM_ORDER_4K);
	paging_map(bs_apic->v_base, msr_read(MSR_APIC_BASE) & APIC_BASE_MASK,
			PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);

	logging_log_info("Initializing Local APIC 0x%X64",
			(uint64_t)*(volatile uint32_t*)(bs_apic->v_base + OFF_APIC_ID));
}

void apic_disab(void) {
	msr_write(MSR_APIC_BASE, msr_read(MSR_APIC_BASE) & ~(uint64_t)APIC_AE);
}
