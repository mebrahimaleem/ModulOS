/* apic_regs.c - APIC registers access */
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

#include <apic/apic_regs.h>

extern uint64_t apic_base;

struct lvt_register_t {
	uint8_t v;
	uint8_t flg;
	uint8_t msk;
	uint8_t resv;
} __attribute__((packed));

void apic_write_reg(enum apic_reg_t reg, uint32_t val) {
	*(volatile uint32_t*)((uint64_t)reg + apic_base) = val;
}

uint32_t apic_read_reg(enum apic_reg_t reg) {
	return *(volatile uint32_t*)((uint64_t)reg + apic_base);
}

void apic_write_lve(enum apic_reg_t reg, uint8_t v, uint8_t flg, uint8_t msk) {
	apic_write_reg(reg, (uint32_t)v + ((uint32_t)flg << 0x8) + ((uint32_t)msk << 0x10));
}

