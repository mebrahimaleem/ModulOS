/* tss.h - task segment state interface */
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

#ifndef KERNEL_CORE_TSS_H
#define KERNEL_CORE_TSS_H

#include <kernel/core/gdt.h>

#define IST_ABORT	1
#define IST_SCHED	2

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

extern void tss_init(volatile struct gdt_t(* gdt)[GDT_NUM_ENTRIES]);

#endif /* KERNEL_CORE_TSS_H */
