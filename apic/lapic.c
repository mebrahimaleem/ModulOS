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

#include <apic/isr.h>
#include <apic/lapic.h>
#include <apic/pic8259.h>
#include <acpi/madt.h>

#define LAPIC_MSR_BASE				0x1B
#define LAPIC_MSR_ENABLE			0x800

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
mutex_t ipi_mutex;

void apic_initlocal() {
	ipi_mutex = kcreateMutex();

	if(acpi_needDisable8259()) {
		pic_mask8259();
	}

	apic_initlocalap(&IDT_BASE);
}

void apic_initlocalap(uint64_t* idt) {
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

	struct IDTD* volatile idtd = (struct IDTD* volatile)idt;

	/* set LVTs */
	union LAPIC_LVT lvt;	
	lvt.zero = 0;
	lvt.cmci.vector = idt_claimIsrVector(LAPIC_CMCI_CODE + ISR_LAPIC_START);
	lvt.cmci.dlvry_mode = LAPIC_DLVRY_FIXED;
	lvt.cmci.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_CMCI_OFF) = lvt;
	idt_installisr(idtd, lvt.cmci.vector, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);

	lvt.zero = 0;
	lvt.timer.vector = idt_claimIsrVector(LAPIC_TIMR_CODE + ISR_LAPIC_START);
	lvt.timer.mask = NOMASK;
	lvt.timer.timr_mode = ONESHOT_TIMER;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_TIMR_OFF) = lvt;
	idt_installisr(idtd, lvt.timer.vector, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);

	lvt.zero = 0;
	lvt.thrm.vector = idt_claimIsrVector(LAPIC_THRM_CODE + ISR_LAPIC_START);
	lvt.thrm.dlvry_mode = LAPIC_DLVRY_FIXED;
	lvt.thrm.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_THRM_OFF) = lvt;
	idt_installisr(idtd, lvt.thrm.vector, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);

	lvt.zero = 0;
	lvt.perfcm.vector = idt_claimIsrVector(LAPIC_PRCR_CODE + ISR_LAPIC_START);
	lvt.perfcm.dlvry_mode = LAPIC_DLVRY_FIXED;
	lvt.perfcm.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_PRCR_OFF) = lvt;
	idt_installisr(idtd, lvt.perfcm.vector, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);

	/* ExtINT */
	lvt.zero = 0;
	lvt.lint0.vector = idt_claimIsrVector(LAPIC_LNT0_CODE + ISR_LAPIC_START);
	lvt.lint0.dlvry_mode = LAPIC_DLVRY_EXTINT;
	lvt.lint0.iipp = IIPP_HIGH;
	lvt.lint0.trig_mode = LAPIC_TRIGMODE_EDGE;
	lvt.lint0.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_LNT0_OFF) = lvt;
	idt_installisr(idtd, lvt.lint0.vector, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);

	/* NMI */
	lvt.zero = 0;
	lvt.lint1.vector = 0x2; // vector 2 for NMIs
	lvt.lint1.dlvry_mode = LAPIC_DLVRY_NMI;
	lvt.lint1.iipp = IIPP_HIGH;
	lvt.lint1.trig_mode = LAPIC_TRIGMODE_EDGE;
	lvt.lint1.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_LNT1_OFF) = lvt;
	/* NMI handler already installed */

	lvt.error.vector = idt_claimIsrVector(LAPIC_EROR_CODE + ISR_LAPIC_START);
	lvt.error.mask = NOMASK;
	*(union LAPIC_LVT* volatile)(lapic_base + LAPIC_LVT_EROR_OFF) = lvt;
	idt_installisr(idtd, lvt.error.vector, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);

	idt_installisr(idtd, LAPIC_SPURIOUS_VECTOR, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);
	/* setup timer */
	//TODO: callibrate clock
	
	/* enable */
	*(uint32_t* volatile)(lapic_base + LAPIC_SVR_OFF) = LAPIC_SPURIOUS_VECTOR | LAPIC_INT_ENABLE;
}

void apic_lapic_sendeoi() {
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
