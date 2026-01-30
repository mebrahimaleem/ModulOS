/* idt.c - interrupt descriptor table */
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

#include <core/idt.h>
#include <core/exceptions.h>
#include <core/cpu_instr.h>
#include <core/tss.h>
#include <core/logging.h>

#include <lib/kmemset.h>

#define IDT_MAX_ENTRY	256

#define IDT_OFF_0_MASK	0xFFFF
#define IDT_OFF_1_MASK	0xFFFF0000
#define IDT_OFF_2_MASK	0xFFFFFFFF00000000

#define IDT_OFF_1_SHFT	16
#define IDT_OFF_2_SHFT	32

#define IDT_TYP_SHFT		5
#define IDT_PRESENT			0x80

struct idt_ptr_t {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

static volatile struct idt_entry_t idt[IDT_MAX_ENTRY] __attribute__((aligned(64)));
static volatile struct idt_ptr_t idt_ptr;
static uint8_t next_vector;

void idt_init(void) {
	kmemset((void*)&idt[0], 0, sizeof(idt));

	idt_install(0x00, (uint64_t)isr_00, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x01, (uint64_t)isr_01, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x02, (uint64_t)isr_02, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x03, (uint64_t)isr_03, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x04, (uint64_t)isr_04, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x05, (uint64_t)isr_05, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x06, (uint64_t)isr_06, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x07, (uint64_t)isr_07, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x08, (uint64_t)isr_08, GDT_CODE_SEL, IST_ABORT, IDT_GATE_INT, 0);
	idt_install(0x09, (uint64_t)isr_09, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x0a, (uint64_t)isr_0a, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x0b, (uint64_t)isr_0b, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x0c, (uint64_t)isr_0c, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x0d, (uint64_t)isr_0d, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x0e, (uint64_t)isr_0e, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x10, (uint64_t)isr_10, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x11, (uint64_t)isr_11, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x12, (uint64_t)isr_12, GDT_CODE_SEL, IST_ABORT, IDT_GATE_INT, 0);
	idt_install(0x13, (uint64_t)isr_13, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x14, (uint64_t)isr_14, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x15, (uint64_t)isr_15, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x1c, (uint64_t)isr_1c, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x1d, (uint64_t)isr_1d, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(0x1e, (uint64_t)isr_1e, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);

	idt_ptr.base = (uint64_t)&idt[0];
	idt_ptr.limit = sizeof(idt) - 1;

	next_vector = 0x30; // leave 0x20 and 0x28 for legacy PIC IRQs

	cpu_lidt((uint64_t)&idt_ptr);
}

void idt_install(
		uint8_t v,
		uint64_t offset,
		uint16_t seg_sel,
		uint8_t ist,
		uint8_t type,
		uint8_t dpl) {
	idt[v].offset0 = offset & IDT_OFF_0_MASK;
	idt[v].offset1 = (offset & IDT_OFF_1_MASK) >> IDT_OFF_1_SHFT;
	idt[v].offset2 = (uint32_t)((offset & IDT_OFF_2_MASK) >> IDT_OFF_2_SHFT);

	idt[v].seg_sel = seg_sel;
	idt[v].ist = ist;
	idt[v].flgs = type | (uint8_t)(dpl << IDT_TYP_SHFT) | IDT_PRESENT;

	logging_log_debug(
			"Installed ISR 0x%X64 (vector) 0x%X64 (isr addr) %u64 (ist) %u64 (cs) 0x%X64 (type) %u64 (dpl)",
			(uint64_t)v, offset, (uint64_t)seg_sel, (uint64_t)ist, (uint64_t)type, (uint64_t)dpl);
}

uint8_t idt_get_vector(void) {
	return next_vector++;
}
