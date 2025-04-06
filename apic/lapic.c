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
#include <core/atomic.h>
#include <core/cpulowlevel.h>
#include <core/IDT.h>
#include <core/serial.h>

#include <apic/lapic.h>
#include <apic/pic8259.h>
#include <acpi/madt.h>

#define LAPIC_MSR_BASE				0x1B
#define LAPIC_MSR_ENABLE			0x800

#define LAPIC_SPURIOUS_VECTOR	0xF0
#define LAPIC_INT_ENABLE			0x100

#define LAPIC_EOI							0x0

#define LAPIC_IDR_OFF					0x20
#define LAPIC_SVR_OFF					0xF0
#define LAPIC_EOI_OFF					0xB0

#define LAPIC_ICR0_OFF				0x300
#define LAPIC_ICR1_OFF				0x310

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

#define NOMASK								0x0
#define ONESHOT_TIMER					0x0
#define IIPP_HIGH							0x0

#define IPI_DLVRY_PENDING			0x1000

uint64_t lapic_base;
uint8_t lapic_spur_vector;
mutex_t ipi_mutex;

void apic_initlocal(void) {
	ipi_mutex = kcreateMutex();

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
	lapic_spur_vector = 0xFF & *(uint32_t* volatile)(lapic_base + LAPIC_SVR_OFF);

	/* set LVTs */
	union LAPIC_LVT lvt;	
	lvt.zero = 0;
	lvt.cmci.vector = LAPIC_CMCI_V;
	lvt.cmci.dlvry_mode = LAPIC_DLVRY_FIXED;
	lvt.cmci.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_CMCI_OFF) = lvt;

	lvt.zero = 0;
	lvt.timer.vector = LAPIC_TIMR_V;
	lvt.timer.mask = NOMASK;
	lvt.timer.timr_mode = ONESHOT_TIMER;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_TIMR_OFF) = lvt;

	lvt.zero = 0;
	lvt.thrm.vector = LAPIC_THRM_V;
	lvt.thrm.dlvry_mode = LAPIC_DLVRY_FIXED;
	lvt.thrm.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_THRM_OFF) = lvt;

	lvt.zero = 0;
	lvt.perfcm.vector = LAPIC_PRCR_V;
	lvt.perfcm.dlvry_mode = LAPIC_DLVRY_FIXED;
	lvt.perfcm.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_PRCR_OFF) = lvt;

	//TODO: parse ACPI to determine whether should be LVL or EDGE triggered
	lvt.zero = 0;
	lvt.lint0.vector = LAPIC_LNT0_V;
	lvt.lint0.dlvry_mode = LAPIC_DLVRY_FIXED;
	lvt.lint0.iipp = IIPP_HIGH;
	lvt.lint0.trig_mode = LAPIC_TRIGMODE_LVL;
	lvt.lint0.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_LNT0_OFF) = lvt;

	lvt.zero = 0;
	lvt.lint1.vector = LAPIC_LNT1_V;
	lvt.lint1.dlvry_mode = LAPIC_DLVRY_FIXED;
	lvt.lint1.iipp = IIPP_HIGH;
	lvt.lint1.trig_mode = LAPIC_TRIGMODE_LVL;
	lvt.lint1.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_LNT1_OFF) = lvt;

	lvt.error.vector = LAPIC_EROR_V;
	lvt.error.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_EROR_OFF) = lvt;

	/* setup timer */
	//TODO: callibrate clock

	/* install ISRs */
	idt_installisr((uint64_t)&ISR_CMCI, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT, LAPIC_CMCI_V);
	idt_installisr((uint64_t)&ISR_TIMR, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT, LAPIC_TIMR_V);
	idt_installisr((uint64_t)&ISR_THRM, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT, LAPIC_THRM_V);
	idt_installisr((uint64_t)&ISR_PRCR, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT, LAPIC_PRCR_V);
	idt_installisr((uint64_t)&ISR_LNT0, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT, LAPIC_LNT0_V);
	idt_installisr((uint64_t)&ISR_LNT1, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT, LAPIC_LNT1_V);
	idt_installisr((uint64_t)&ISR_EROR, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT, LAPIC_EROR_V);
	idt_installisr((uint64_t)&ISR_SPUR, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT, lapic_spur_vector);
	
	/* enable */
	*(uint32_t* volatile)(lapic_base + LAPIC_SVR_OFF) = LAPIC_SPURIOUS_VECTOR | LAPIC_INT_ENABLE;

}

void apic_lapic_ISRHandler(uint64_t code) {
	serialPrintf(SERIAL1, "GOT 0x%x\n", code);
	
	union LAPIC_LVT lvt;
	switch (code) {
		case LAPIC_CMCI_V:
			lvt = *(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_CMCI_OFF);
			break;
		case LAPIC_TIMR_V:
			lvt = *(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_TIMR_OFF);
			break;
		case LAPIC_THRM_V:
			lvt = *(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_THRM_OFF);
			break;
		case LAPIC_PRCR_V:
			lvt = *(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_PRCR_OFF);
			break;
		case LAPIC_LNT0_V:
			lvt = *(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_LNT0_OFF);
			break;
		case LAPIC_LNT1_V:
			lvt = *(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_LNT1_OFF);
			break;
		case LAPIC_EROR_V:
			lvt = *(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_EROR_OFF);
			break;
		case LAPIC_SPUR_V:
			return;
		default:
			panic(KPANIC_APIC);
	}
	
	// check if no eoi
	if (code != LAPIC_TIMR_V && (
		lvt.cmci.dlvry_mode == LAPIC_DLVRY_NMI ||
		lvt.cmci.dlvry_mode == LAPIC_DLVRY_SMI ||
		lvt.cmci.dlvry_mode == LAPIC_DLVRY_INIT ||
		lvt.cmci.dlvry_mode == LAPIC_DLVRY_EXTINIT ||
		lvt.cmci.dlvry_mode == LAPIC_DLVRY_STARTUP
	)) {
		return;
	}

	/* send eoi */
	*(uint32_t* volatile)(lapic_base + LAPIC_EOI_OFF) = LAPIC_EOI;
}

void apic_lapic_sendipi(uint8_t v, uint32_t flg, uint8_t dest) {
	kacquireMutex(ipi_mutex);
	apic_lapic_waitForIpi();
	*(uint32_t* volatile)(lapic_base + LAPIC_ICR1_OFF) = dest << 24;
	*(uint32_t* volatile)(lapic_base + LAPIC_ICR0_OFF) = v | flg;
	kreleaseMutex(ipi_mutex);
}

void apic_lapic_waitForIpi(void) {
	while ((*(uint32_t* volatile)(lapic_base + LAPIC_ICR0_OFF) & IPI_DLVRY_PENDING) == IPI_DLVRY_PENDING);
}

uint8_t apic_getId() {
	return (uint8_t)(*(uint32_t* volatile)(lapic_base + LAPIC_IDR_OFF) >> 24);
}

#endif /* APIC_LAPIC_C */
