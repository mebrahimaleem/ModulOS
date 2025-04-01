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

#define LAPIC_LVT_CMCI_OFF		0x2F0
#define LAPIC_LVT_TIMR_OFF		0x320
#define LAPIC_LVT_THRM_OFF		0x330
#define LAPIC_LVT_PRCR_OFF		0x340
#define LAPIC_LVT_LNT0_OFF		0x350
#define LAPIC_LVT_LNT1_OFF		0x360
#define LAPIC_LVT_EROR_OFF		0x370

#define MAXLA_LEAF						0x80000000
#define MAXPA_LEAF						0x80000008
#define CLOCK_LEAF						0x15

#define DLVRY_FIXED						0x0
#define NOMASK								0x0
#define ONESHOT_TIMER					0x0
#define IIPP_HIGH							0x0
#define TRIGMODE_EDGE					0x0
#define TRIGMODE_LVL					0x1

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

	/* disable while being set up */
	*(uint32_t* volatile)(lapic_base + LAPIC_SVR_OFF) = LAPIC_SPURIOUS_VECTOR;

	/* set LVTs */
	union LAPIC_LVT lvt;	
	lvt.zero = 0;
	lvt.cmci.vector = LAPIC_CMCI_V;
	lvt.cmci.dlvry_mode = DLVRY_FIXED;
	lvt.cmci.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_CMCI_OFF) = lvt;

	lvt.zero = 0;
	lvt.timer.vector = LAPIC_TIMR_V;
	lvt.timer.mask = NOMASK;
	lvt.timer.timr_mode = ONESHOT_TIMER;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_TIMR_OFF) = lvt;

	lvt.zero = 0;
	lvt.thrm.vector = LAPIC_THRM_V;
	lvt.thrm.dlvry_mode = DLVRY_FIXED;
	lvt.thrm.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_THRM_OFF) = lvt;

	lvt.zero = 0;
	lvt.perfcm.vector = LAPIC_PRCR_V;
	lvt.perfcm.dlvry_mode = DLVRY_FIXED;
	lvt.perfcm.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_PRCR_OFF) = lvt;

	lvt.zero = 0;
	lvt.lint0.vector = LAPIC_LNT0_V;
	lvt.lint0.dlvry_mode = DLVRY_FIXED;
	lvt.lint0.iipp = IIPP_HIGH;
	lvt.lint0.trig_mode = TRIGMODE_EDGE; // LINT0 for edge
	lvt.lint0.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_LNT0_OFF) = lvt;

	lvt.zero = 0;
	lvt.lint1.vector = LAPIC_LNT1_V;
	lvt.lint1.dlvry_mode = DLVRY_FIXED;
	lvt.lint1.iipp = IIPP_HIGH;
	lvt.lint1.trig_mode = TRIGMODE_LVL; // LINT1 for level
	lvt.lint1.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_LNT1_OFF) = lvt;

	lvt.error.vector = LAPIC_EROR_V;
	lvt.error.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_EROR_OFF) = lvt;

	/* setup timer */
	//TODO: callibrate clock
	
	/* enable */
	*(uint32_t* volatile)(lapic_base + LAPIC_SVR_OFF) = LAPIC_SPURIOUS_VECTOR | LAPIC_INT_ENABLE;

	lapic_spur_vector = 0xFF & *(uint32_t* volatile)(lapic_base + LAPIC_SVR_OFF);
}

#endif /* APIC_LAPIC_C */
