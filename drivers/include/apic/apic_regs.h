/* apic_regs.h - APIC registers interface */
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

#ifndef DRIVERS_APIC_APIC_REGS_H
#define DRIVERS_APIC_APIC_REGS_H

#include <stdint.h>

#define APIC_EOI 0x00

enum apic_reg_t {
	APIC_REG_IDR = 0x0020, // id register
	APIC_REG_VER = 0x0030, // version register
	APIC_REG_EOI = 0x00B0, // end of interrupt register
	APIC_REG_SPR = 0x00F0, // spurious interrupt register
	APIC_REG_ESR = 0x0270, // error status register
	APIC_REG_TME = 0x0320, // timer LVT entry register
	APIC_REG_THE = 0x0330, // thermal LVT entry register
	APIC_REG_PRE = 0x0340, // performance LVT entry register
	APIC_REG_L0E = 0x0350, // LINT0 LVT entry register
	APIC_REG_L1E = 0x0360, // LINT1 LVT entry register
	APIC_REG_ERE = 0x0370, // error LVT entry register
	APIC_REG_ICR = 0x0380, // timer initial count register
	APIC_REG_DCR = 0x03E0, // divide configuration register
	APIC_REG_EFR = 0x0400, // extended feature register
	APIC_REG_EE0 = 0x0500, // extened lVT entry register 0
	APIC_REG_EE1 = 0x0510, // extened lVT entry register 1
	APIC_REG_EE2 = 0x0520, // extened lVT entry register 2
	APIC_REG_EE3 = 0x0530, // extened lVT entry register 3
};

extern void apic_write_reg(enum apic_reg_t reg, uint32_t val);
extern uint32_t apic_read_reg(enum apic_reg_t reg);

extern void apic_write_lve(enum apic_reg_t reg, uint8_t v, uint8_t flg, uint8_t msk);

#endif /* DRIVERS_APIC_APIC_REGS_H */
