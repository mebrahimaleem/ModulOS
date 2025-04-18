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
#include <core/portio.h>
#include <core/idt.h>
#include <core/scheduler.h>
#include <core/memory.h>

#include <apic/calibrate.h>
#include <apic/isr.h>
#include <apic/lapic.h>
#include <apic/pic8259.h>
#include <apic/ioapic.h>
#include <apic/pit.h>

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

#define LAPIC_TMR_DIV_OFF			0x3E0
#define LAPIC_TMR_DIV					0xB
#define LAPIC_ICD_OFF					0x380

#define TIMER_INITIAL					0xFFFFFFFF

#define MAXLA_LEAF						0x80000000
#define MAXPA_LEAF						0x80000008
#define CLOCK_LEAF						0x15

#define NOMASK								0x0
#define ONESHOT_TIMER					0x0
#define IIPP_HIGH							0x0

#define IPI_DLVRY_PENDING			0x1000

#define TIMER_DISABLE					0x0

#define IPI_FLUSH_INDEX				0x0

/* no need to keep track of fraction since PIT is not precise enough */
#define PIT_PERIOD_US					10000

uint64_t lapic_base;
mutex_t ipi_mutex;

uint8_t lapic_isrs[6];
uint8_t ipi_isrs[1];

uint8_t lapic_init = 0;

struct cpu_specific* lapic_percpu[256];

void apic_initlocal() {
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

	/* set page to no cache and write through */
	paging_changeFlags(kPML4T, (void*)lapic_base, KMEM_PAGE_PRESENT | KMEM_PAGE_WRITE | KMEM_PAGE_WT | KMEM_PAGE_NOCACHE, PAGE_GRANULARITY_4K);

	lapic_isrs[0] = idt_claimIsrVector(LAPIC_CMCI_CODE + ISR_LAPIC_START);
	lapic_isrs[1] = idt_claimIsrVector(LAPIC_TIMR_CODE + ISR_LAPIC_START);
	lapic_isrs[2] = idt_claimIsrVector(LAPIC_THRM_CODE + ISR_LAPIC_START);
	lapic_isrs[3] = idt_claimIsrVector(LAPIC_PRCR_CODE + ISR_LAPIC_START);
	lapic_isrs[4] = idt_claimIsrVector(LAPIC_LNT0_CODE + ISR_LAPIC_START);
	lapic_isrs[5] = idt_claimIsrVector(LAPIC_EROR_CODE + ISR_LAPIC_START);

	ipi_isrs[IPI_FLUSH_INDEX] = idt_claimIsrVector(LAPIC_IPI_FLUSH_CODE + ISR_LAPIC_START);

	apic_installisrs((volatile struct IDTD*)&IDT_BASE);
	apic_initlocalap();

	lapic_init = 1;
}

void apic_installisrs(volatile struct IDTD* idtd) {
	idt_installisr(idtd, lapic_isrs[0], 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);
	idt_installisr(idtd, lapic_isrs[1], 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);
	idt_installisr(idtd, lapic_isrs[2], 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);
	idt_installisr(idtd, lapic_isrs[3], 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);
	idt_installisr(idtd, lapic_isrs[4], 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);
	idt_installisr(idtd, lapic_isrs[5], 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);
	idt_installisr(idtd, LAPIC_SPURIOUS_VECTOR, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);

	idt_installisr(idtd, ipi_isrs[IPI_FLUSH_INDEX], 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);
}

void apic_initlocalap() {
	/* disable while being set up */
	*(volatile uint32_t* )(lapic_base + LAPIC_SVR_OFF) = LAPIC_SPURIOUS_VECTOR;

	/* set LVTs */
	union LAPIC_LVT lvt;	
	lvt.zero = 0;
	lvt.cmci.vector = lapic_isrs[0];
	lvt.cmci.dlvry_mode = LAPIC_DLVRY_FIXED;
	lvt.cmci.mask = NOMASK;
	*(volatile union LAPIC_LVT* )(lapic_base + LAPIC_LVT_CMCI_OFF) = lvt;

	lvt.zero = 0;
	lvt.timer.vector = lapic_isrs[1];
	lvt.timer.mask = NOMASK;
	lvt.timer.timr_mode = ONESHOT_TIMER;
	*(volatile union LAPIC_LVT* )(lapic_base + LAPIC_LVT_TIMR_OFF) = lvt;

	lvt.zero = 0;
	lvt.thrm.vector = lapic_isrs[2];
	lvt.thrm.dlvry_mode = LAPIC_DLVRY_FIXED;
	lvt.thrm.mask = NOMASK;
	*(volatile union LAPIC_LVT* )(lapic_base + LAPIC_LVT_THRM_OFF) = lvt;

	lvt.zero = 0;
	lvt.perfcm.vector = lapic_isrs[3];
	lvt.perfcm.dlvry_mode = LAPIC_DLVRY_FIXED;
	lvt.perfcm.mask = NOMASK;
	*(volatile union LAPIC_LVT* )(lapic_base + LAPIC_LVT_PRCR_OFF) = lvt;

	/* ExtINT */
	lvt.zero = 0;
	lvt.lint0.vector = lapic_isrs[4];
	lvt.lint0.dlvry_mode = LAPIC_DLVRY_EXTINT;
	lvt.lint0.iipp = IIPP_HIGH;
	lvt.lint0.trig_mode = LAPIC_TRIGMODE_EDGE;
	lvt.lint0.mask = NOMASK;
	*(volatile union LAPIC_LVT* )(lapic_base + LAPIC_LVT_LNT0_OFF) = lvt;

	/* NMI */
	lvt.zero = 0;
	lvt.lint1.vector = 0x2; // vector 2 for NMIs
	lvt.lint1.dlvry_mode = LAPIC_DLVRY_NMI;
	lvt.lint1.iipp = IIPP_HIGH;
	lvt.lint1.trig_mode = LAPIC_TRIGMODE_EDGE;
	lvt.lint1.mask = NOMASK;
	*(volatile union LAPIC_LVT* )(lapic_base + LAPIC_LVT_LNT1_OFF) = lvt;
	/* NMI handler already installed */

	lvt.error.vector = lapic_isrs[5];
	lvt.error.mask = NOMASK;
	*(volatile union LAPIC_LVT* )(lapic_base + LAPIC_LVT_EROR_OFF) = lvt;

	/* disable timer */
	*(volatile uint32_t* )(lapic_base + LAPIC_ICD_OFF) = TIMER_DISABLE;
	*(volatile uint32_t* )(lapic_base + LAPIC_TMR_DIV_OFF) = LAPIC_TMR_DIV;
	
	/* enable */
	*(volatile uint32_t* )(lapic_base + LAPIC_SVR_OFF) = LAPIC_SPURIOUS_VECTOR | LAPIC_INT_ENABLE;

	/* create cpuspecific entry */
	lapic_percpu[apic_getId()] = kmalloc(sizeof(struct cpu_specific));
}

void apic_lapic_sendeoi() {
	*(volatile uint32_t* )(lapic_base + LAPIC_EOI_OFF) = LAPIC_EOI;
}

void apic_lapic_sendipi(uint8_t v, uint32_t flg, uint8_t dest) {
	kacquireMutex(ipi_mutex);
	apic_lapic_waitForIpi();
	*(volatile uint32_t* )(lapic_base + LAPIC_ICR1_OFF) = dest << 24;
	*(volatile uint32_t* )(lapic_base + LAPIC_ICR0_OFF) = v | flg;
	kreleaseMutex(ipi_mutex);
}


void apic_lapic_waitForIpi() {
	while ((*(volatile uint32_t* )(lapic_base + LAPIC_ICR0_OFF) & IPI_DLVRY_PENDING) == IPI_DLVRY_PENDING)
		pause();
}

uint8_t apic_getId() {
	return (uint8_t)(*(volatile uint32_t* )(lapic_base + LAPIC_IDR_OFF) >> 24);
}

void apic_lapic_calibrateTimer() {
	//TODO: recalibrate on PState change
	kacquireMutex(pit_mutex);

	const uint32_t gsi = apic_translateGsi(IOAPIC_ISA_PIT);
	const uint8_t apicId = apic_getId();

	apic_maskIrq(gsi);
	/* swtich to low latency ISR for high precision calibration */
	idt_installcustomisr((volatile struct IDTD*)&IDT_BASE, (uint64_t)&lowlatency_isr, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT, apic_pitisr);

	/* switch to generic ISR to avoid accidently schduling */
	idt_installisr((volatile struct IDTD*)&IDT_BASE, lapic_isrs[1], 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);

	/* change IOAPIC PIT IRQ to local APIC */
	apic_changeTarget(gsi, apicId);

	/* transfer to asm for low latency measurements */
	const uint64_t period = TIMER_INITIAL - (uint32_t)calibrate_lapic_timer(gsi, lapic_base);

	/* swtich back to normal ISR */
	idt_installisr((volatile struct IDTD*)&IDT_BASE, apic_pitisr, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);

	idt_installcustomisr((volatile struct IDTD*)&IDT_BASE, (uint64_t)&scheduler_isr, 7, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT, lapic_isrs[1]);
	
	apic_unmaskIrq(gsi);

	kreleaseMutex(pit_mutex);

	/*
	* ApicPeriod apicUnits = PitPeriod ms
	* 1 = ApicPeriod apicUnits / PitPeriod us
	*
	* eg. so if we want a delay of 200us in apic timer, we set countdown to
	*		200us * (ApicPeriod apicUnits / PitPeriod us) = x apicUnits
	*/

	lapic_percpu[apicId]->calib_whole = period / PIT_PERIOD_US;
	lapic_percpu[apicId]->calib_frac = period % PIT_PERIOD_US;
}

void apic_setTimerDeadline(uint64_t us) {
	const uint8_t apicid = apic_getId();
	const uint32_t deadline = (uint32_t)(us * lapic_percpu[apicid]->calib_whole) + (uint32_t)(us * lapic_percpu[apicid]->calib_frac / PIT_PERIOD_US);
	*(volatile uint32_t*)(lapic_base + LAPIC_ICD_OFF) = deadline;
}

void apic_lapic_sendipi_flush() {
	if (!lapic_init) return;

	apic_lapic_sendipi(ipi_isrs[IPI_FLUSH_INDEX], LAPIC_IPI_FIXED | LAPIC_IPI_PHYSC | LAPIC_IPI_ASSERT | LAPIC_IPI_EDGE | LAPIC_IPI_ALLEX, 0);
}

#endif /* APIC_LAPIC_C */
