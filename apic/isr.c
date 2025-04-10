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
#include <core/idt.h>

#include <apic/lapic.h>
#include <apic/ioapic.h>
#include <apic/isr.h>

void isr_handler(uint64_t code) {
	/* check for spurious */
	if (code == LAPIC_SPURIOUS_VECTOR) {
		return;
	}

	uint64_t internal = idt_translateCode(code);

	DEBUG_LOGF("ISR Vector: 0x%lx Internal 0x%lx", code, internal);

	// check if no eoi
	if (internal == ISR_LAPIC_START + LAPIC_LNT0_CODE) {
		return;
	}

	/* send eoi */
	apic_lapic_sendeoi();
}

#endif /* APIC_ISR_C */
