/* tss.c - task segment state */
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

#include <core/tss.h>
#include <core/alloc.h>
#include <core/cpu_instr.h>
#include <core/kentry.h>
#include <core/logging.h>
#include <core/gdt.h>
#include <core/proc_data.h>

#include <lib/kmemset.h>

#define IST_ABORT_SIZE	0x1000
#define IST_SCHED_SIZE	0x1000

#define IST_LO_MASK			0xFFFFFFFF
#define IST_HI_SHFT			32

void tss_init(volatile struct gdt_t(* gdt)[GDT_NUM_ENTRIES]) {
	volatile struct tss_t* tss = kmalloc(sizeof(struct tss_t));

	kmemset((void*)tss, 0, sizeof(struct tss_t));

	proc_data_get()->tss = tss;

	const uint64_t ist1 = (uint64_t)kmalloc(IST_ABORT_SIZE) + IST_ABORT_SIZE;
	tss->ist1_lo = ist1 & IST_LO_MASK;
	tss->ist1_hi = (uint32_t)(ist1 >> IST_HI_SHFT);

	const uint64_t ist2 = (uint64_t)kmalloc(IST_SCHED_SIZE) + IST_SCHED_SIZE;
	tss->ist2_lo = ist2 & IST_LO_MASK;
	tss->ist2_hi = (uint32_t)(ist2 >> IST_HI_SHFT);

	logging_log_debug("New TSS @ 0x%lX - 0x%lX 0x%lX (ist1)",
			(uint64_t)tss, ist1);

	((struct gdt_sys_t*)&(*gdt)[GDT_TSS_INDEX])->base0 = 
		(uint64_t)tss & GDT_BASE0_MASK;

	((struct gdt_sys_t*)&(*gdt)[GDT_TSS_INDEX])->base1 = 
		((uint64_t)tss & GDT_BASE1_MASK) >> GDT_BASE1_SHFT;

	((struct gdt_sys_t*)&(*gdt)[GDT_TSS_INDEX])->base2 = 
		((uint64_t)tss & GDT_BASE2_MASK) >> GDT_BASE2_SHFT;

	((struct gdt_sys_t*)&(*gdt)[GDT_TSS_INDEX])->base3 = 
		(uint32_t)(((uint64_t)tss & GDT_BASE3_MASK) >> GDT_BASE3_SHFT);

	((struct gdt_sys_t*)&(*gdt)[GDT_TSS_INDEX])->limit0 = 
		(sizeof(*tss) - 1) & GDT_LIMIT0_MASK;

	((struct gdt_sys_t*)&(*gdt)[GDT_TSS_INDEX])->flglmt1 = 
		(((sizeof(*tss) - 1) & GDT_LIMIT1_MASK) >> GDT_LIMIT1_SHFT);

	((struct gdt_sys_t*)&(*gdt)[GDT_TSS_INDEX])->access =
		GDT_ACS_TSS | GDT_ACS_P;

	cpu_ltr_28();
}
