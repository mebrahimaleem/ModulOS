/* hpet_init.c - hpet init */
/* Copyright (C) 2026  Ebrahim Aleem
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
#include <kernel/core/alloc.h>
#include <kernel/core/clock_src.h>

#define HPET_GROUP_ENA		0x1
#define HPET_INT_ENA			0x4
#define HPET_NUM_TIM			0x1F00
#define HPET_NUM_TIM_SHF	8
#define HPET_PER					0xFFFFFFFF00000000
#define HPET_PER_SHF			32

#define NS_TO_FS	1000000

struct hpet_timer_t {
	uint64_t cap;
	uint64_t comp;
	uint64_t fsb;
	uint64_t r0;
} __attribute__((packed));

static volatile struct hpet_group_t {
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

static uint64_t hpet_get_counter(void* meta) {
	volatile struct hpet_group_t* hpet = meta;
	return hpet->counter;
}

static void hpet_set_counter(void* meta, uint64_t counter) {
	volatile struct hpet_group_t* hpet = meta;
	hpet->gen_conf &= ~(uint64_t)HPET_GROUP_ENA;
	hpet->counter = counter;
	hpet->gen_conf |= HPET_GROUP_ENA;
}

void hpet_init(void) {
	acpi_get_hpet_bases((uint64_t*)&hpet_reg_bases[0]);

	struct clock_src_t* clock;

	int8_t num_tim;

	for (uint8_t i = 0; i < 8; i++) {
		if (hpet_reg_bases[i]) {
			logging_log_debug("Found HPET register base @ 0x%lx", hpet_reg_bases[i]);
			const uint64_t temp = mm_alloc_dv(MM_ORDER_4K);
			paging_map(
					temp,
					(uint64_t)hpet_reg_bases[i],
					PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K,
					PAGE_4K);
			hpet_reg_bases[i] = (void*)temp;

			// disable all interrupts
			for (num_tim = (hpet_reg_bases[i]->cap & HPET_NUM_TIM) >> HPET_NUM_TIM_SHF;
					num_tim >= 0;
					num_tim--) {
				hpet_reg_bases[i]->timers[num_tim].cap &= ~(uint64_t)HPET_INT_ENA;
			}


			// enable timers with no interrupts
			clock = kmalloc(sizeof(struct clock_src_t));
			clock->counter = hpet_get_counter;
			clock->reset = hpet_set_counter;
			clock->meta = (void*)hpet_reg_bases[i];
			clock->period_fs = (hpet_reg_bases[i]->cap & HPET_PER) >> HPET_PER_SHF;
			clock_src_register(clock);

			clock->reset(clock->meta, 0);
		}
	}
}
