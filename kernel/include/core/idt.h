/* idt.h - interrupt descriptor table interface */
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

#ifndef KERNEL_CORE_IDT_H
#define KERNEL_CORE_IDT_H

#include <stdint.h>

#define IDT_GATE_INT	0xE
#define IDT_GATE_TRP	0xF

#define GDT_CODE_SEL		8

struct idt_entry_t {
	uint16_t offset0;
	uint16_t seg_sel;
	uint8_t ist;
	uint8_t flgs;
	uint16_t offset1;
	uint32_t offset2;
	uint32_t resv;
} __attribute__((packed));

extern void idt_init(void);

extern void idt_install(
		uint8_t v,
		uint64_t offset,
		uint16_t seg_sel,
		uint8_t ist,
		uint8_t type,
		uint8_t dpl);

extern uint8_t idt_get_vector(void);

#endif /* KERNEL_CORE_IDT_H */
