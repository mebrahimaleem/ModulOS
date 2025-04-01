/* lapic.c - Local APIC routines */
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

#ifndef APIC_LAPIC_C
#define APIC_LAPIC_C

#include <core/panic.h>
#include <core/cpulowlevel.h>

#include <apic/lapic.h>
#include <apic/pic8259.h>
#include <acpi/madt.h>

#define LAPIC_MSR_BASE				0x1B
#define LAPIC_MSR_ENABLE			0x800

#define LAPIC_SPURIOUS_VECTOR	0xF0
#define LAPIC_INT_ENABLE			0x100

#define LAPIC_SVR_OFF					0xF0

#define MAXLA_LEAF						0x80000000
#define MAXPA_LEAF						0x80000008

uint64_t lapic_base;
uint8_t lapic_spur_vector;

void apic_initlocal(void) {
	if(acpi_needDisable8259()) {
		pic_mask8259();
	}


	uint32_t t0 = 0, t1 = 0;
	uint64_t maxpab = 36;
	/* check for max address bits */
	cpuid(MAXLA_LEAF, 0, &t0, &t1, &t1, &t1);
	if (t0 >= MAXPA_LEAF) {
		cpuid(MAXPA_LEAF, 0, &t0, &t1, &t1, &t1);
		maxpab = t0 & 0xFF;
	}
	const uint64_t addrmask = ((uint64_t)2 << maxpab) - 1;

	/* enable and set base */

	cpuGetMSR(LAPIC_MSR_BASE, &t0, &t1);

	lapic_base = addrmask & ((t0 & 0xfffff000) + ((uint64_t)t1 << 32));

	cpuSetMSR(LAPIC_MSR_BASE, (uint32_t)lapic_base | LAPIC_MSR_ENABLE, (uint32_t)(lapic_base >> 32));

	/* set spurious interrupt register */
	*(uint32_t* volatile)(lapic_base + LAPIC_SVR_OFF) = LAPIC_SPURIOUS_VECTOR | LAPIC_INT_ENABLE;

	lapic_spur_vector = 0xFF & *(uint32_t* volatile)(lapic_base + LAPIC_SVR_OFF);
}

#endif /* APIC_LAPIC_C */
