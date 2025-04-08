/* isr.c - interrupt service routines */
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

#ifndef APIC_ISR_C
#define APIC_ISR_C

#include <stdint.h>

#include <core/serial.h>
#include <core/IDT.h>

#include <apic/lapic.h>
#include <apic/ioapic.h>
#include <apic/isr.h>

void isr_handler(uint64_t code) {
	/* check for spurious */
	if (code == LAPIC_SPURIOUS_VECTOR) {
		return;
	}

#ifdef DEBUG
	serialPrintf(SERIAL1, "ISR: Vector: 0x%x Internal: 0x", code + 0x20);
#endif

	code = idt_translateCode(code);

#ifdef DEBUG
	serialPrintf(SERIAL1, "%x\n", code);
#endif

	// check if no eoi
	if (code == ISR_LAPIC_START + LAPIC_LNT0_CODE) {
		return;
	}

	/* send eoi */
	apic_lapic_sendeoi();
}

#endif /* APIC_ISR_C */
