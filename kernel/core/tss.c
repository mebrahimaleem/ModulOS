/* tss.c - task segment state */
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

#include "core/gdt.h"
#include <stdint.h>

#include <core/tss.h>
#include <core/alloc.h>
#include <core/cpu_instr.h>
#include <core/kentry.h>

#include <lib/mem_utils.h>

#define RSP_SIZE				0x1000
#define IST_ABORT_SIZE	0x1000

#define IST_LO_MASK			0xFFFFFFFF
#define IST_HI_SHFT			32

struct tss_t {
	uint32_t resv0;
	uint32_t rsp0_lo;
	uint32_t rsp0_hi;
	uint32_t rsp1_lo;
	uint32_t rsp1_hi;
	uint32_t rsp2_lo;
	uint32_t rsp2_hi;
	uint32_t resv1;
	uint32_t resv2;
	uint32_t ist1_lo;
	uint32_t ist1_hi;
	uint32_t ist2_lo;
	uint32_t ist2_hi;
	uint32_t ist3_lo;
	uint32_t ist3_hi;
	uint32_t ist4_lo;
	uint32_t ist4_hi;
	uint32_t ist5_lo;
	uint32_t ist5_hi;
	uint32_t ist6_lo;
	uint32_t ist6_hi;
	uint32_t ist7_lo;
	uint32_t ist7_hi;
	uint32_t resv3;
	uint32_t resv4;
	uint16_t resv5;
	uint16_t iopb;
} __attribute__((packed));


void tss_init(void) {
	volatile struct tss_t* tss = kmalloc(sizeof(struct tss_t));

	memset((void*)tss, 0, sizeof(struct tss_t));

	const uint64_t rsp0 = (uint64_t)kmalloc(RSP_SIZE);
	tss->rsp0_lo = rsp0 & IST_LO_MASK;
	tss->rsp0_hi = (uint32_t)(rsp0 >> IST_HI_SHFT);

	//TODO: setup ists as needed
	
	const uint64_t ist1 = (uint64_t)kmalloc(IST_ABORT_SIZE);
	tss->ist1_lo = ist1 & IST_LO_MASK;
	tss->ist1_hi = (uint32_t)(ist1 >> IST_HI_SHFT);

	((struct gdt_sys_t*)&(*boot_context.gdt)[GDT_TSS_INDEX])->base0 = 
		(uint64_t)tss & GDT_BASE0_MASK;

	((struct gdt_sys_t*)&(*boot_context.gdt)[GDT_TSS_INDEX])->base1 = 
		((uint64_t)tss & GDT_BASE1_MASK) >> GDT_BASE1_SHFT;

	((struct gdt_sys_t*)&(*boot_context.gdt)[GDT_TSS_INDEX])->base2 = 
		((uint64_t)tss & GDT_BASE2_MASK) >> GDT_BASE2_SHFT;

	((struct gdt_sys_t*)&(*boot_context.gdt)[GDT_TSS_INDEX])->base3 = 
		(uint32_t)(((uint64_t)tss & GDT_BASE3_MASK) >> GDT_BASE3_SHFT);

	((struct gdt_sys_t*)&(*boot_context.gdt)[GDT_TSS_INDEX])->limit0 = 
		(sizeof(*tss) - 1) & GDT_LIMIT0_MASK;

	((struct gdt_sys_t*)&(*boot_context.gdt)[GDT_TSS_INDEX])->flglmt1 = 
		(((sizeof(*tss) - 1) & GDT_LIMIT1_MASK) >> GDT_LIMIT1_SHFT);

	((struct gdt_sys_t*)&(*boot_context.gdt)[GDT_TSS_INDEX])->access =
		GDT_ACS_TSS | GDT_ACS_P;

	cpu_ltr_28();
}
