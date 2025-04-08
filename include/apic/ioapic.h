/* ioapic.h - IO APIC routines */
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

#ifndef APIC_IOAPIC_H
#define APIC_IOAPIC_H

#define IOAPIC_ISA_PIT	0x0
#define IOAPIC_ISA_KBD	0x1
#define IOAPIC_ISA_RTC	0x8

struct apic_ioapic {
	uint32_t mingsi;
	uint32_t maxgsi;
	uint32_t base;
	struct apic_ioapic* next;
};

void apic_initio(void);

/* translates an IRQ code to gsi */
uint32_t apic_translateGsi(uint64_t code);

void apic_maskIrq(uint32_t gsi);

#endif /* APIC_IOAPIC_H */
