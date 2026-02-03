/* hpet.c - hpet init */
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

#include <hpet/hpet_init.h>
#include <acpi/tables.h>

#include <kernel/core/logging.h>
#include <kernel/core/panic.h>
#include <kernel/core/paging.h>
#include <kernel/core/mm.h>

#define HPET_GROUP_ENA		0x1

struct hpet_timer_t {
	uint64_t cap;
	uint64_t comp;
	uint64_t fsb;
	uint64_t r0;
} __attribute__((packed));

static volatile struct {
	uint64_t cap;
	uint64_t r0;
	uint64_t gen_conf;
	uint64_t r1;
	uint64_t gen_int;
	uint64_t r2[25];
	uint64_t counter;
	uint64_t r3;
	struct hpet_timer_t timers[32];
} __attribute__((packed))* hpet_reg_bases[8];

void hpet_init(void) {
	acpi_get_hpet_bases((uint64_t*)&hpet_reg_bases[0]);

	for (uint8_t i = 0; i < 8; i++) {
		if (hpet_reg_bases[i]) {
			logging_log_debug("Found HPET register base @ 0x%x64", hpet_reg_bases[i]);
			const uint64_t temp = mm_alloc_dv(MM_ORDER_4K);
			paging_map(
					temp,
					(uint64_t)hpet_reg_bases[i],
					PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K,
					PAGE_4K);
			hpet_reg_bases[i] = (void*)temp;

			// disable all hpet timer groups
			hpet_reg_bases[i]->gen_conf &= ~(uint64_t)HPET_GROUP_ENA;
		}
	}
}
