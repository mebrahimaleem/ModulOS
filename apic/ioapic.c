/* ioapic.c - IO APIC routines */
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

#ifndef APIC_IOAPIC_C
#define APIC_IOAPIC_C

#include <stdint.h>

#include <core/IDT.h>
#include <core/panic.h>
#include <core/atomic.h>
#include <core/memory.h>

#include <acpi/madt.h>

#include <apic/isr.h>
#include <apic/lapic.h>
#include <apic/ioapic.h>

#define IOREGSEL_OFF	0x00
#define IOWIN_OFF			0x10

#define IOAPICVER			0x01
#define IOREDTBL_ST		(uint32_t)0x10

#define LO_MASK				0xFFFE5000
#define HI_MASK				0x00FFFFFF

#define DLVRY_FIXED		0x00000
#define DEST_PHYSCL		0x00000
#define INTOL_LOWA		0x02000
#define TRIG_LEVEL		0x08000
#define MASK_INT			0x10000

#define OVER_LOA			0x3
#define OVER_TRL			0x3

uint32_t irq_remaps_inv[16]; //only 16 isa irqs

struct apic_ioapic* ioapics;
mutex_t ioapic_mutex;

void apic_initio() {
	ioapic_mutex = kcreateMutex();
	ioapics = 0;
	uint64_t hint = 0;
	uint32_t i;

	/* first iterate through source overrides to find correct config */
	uint32_t* flags = kmalloc(4096 * 4);
	for (i = 0; i < 16; i++) {
		flags[i] = 0; //ISA default is active hi edge triggered
		irq_remaps_inv[i] = i;
	}
	for (i = 16; i < 4096; i++) {
		flags[i] = INTOL_LOWA | TRIG_LEVEL;
	}

	struct acpi_overrides* overrides;
	while (1) {
		hint = acpi_nextMADT(MADT_REDIR_TYPE, hint, (struct acpi_madt_ic**)&overrides);
		
		if (hint == 0) {
			break;
		}

		if (overrides->gsi >= 4096 || overrides->source >= 16) {
			/* not supported */
			panic(KPANIC_APIC);
		}
		
		flags[overrides->gsi] = 0;
		irq_remaps_inv[overrides->source] = overrides->gsi;

		if (overrides->polarity == OVER_LOA) {
			flags[overrides->gsi] |= INTOL_LOWA;
		}
		
		if (overrides->polarity == OVER_TRL) {
			flags[overrides->gsi] |= TRIG_LEVEL;
		}
	}

	/* iterate through all IO APICs and initialize them */
	hint = 0;
	struct acpi_ioapic* ioapic;

	uint32_t lo;
	uint32_t hi;
	uint8_t isr;

	struct apic_ioapic* tapic;

	while (1) {
		hint = acpi_nextMADT(MADT_IOAPIC_TYPE, hint, (struct acpi_madt_ic**)&ioapic);
		
		if (hint == 0) {
			break;
		}

		*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOAPICVER;
		const uint8_t maxreds = (uint8_t)(*(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) >> 16);

		tapic = kmalloc(sizeof(struct apic_ioapic));
		tapic->mingsi = ioapic->intstart;
		tapic->maxgsi = ioapic->intstart + maxreds + 1;
		tapic->base = ioapic->base;
		tapic->next = ioapics;
		ioapics = tapic;
		kfree(tapic);
		
		for (i = 0; i <= maxreds; i++) {
			isr = idt_claimIsrVector(i + ioapic->intstart + ISR_IOAPIC_START);

			if (isr == 0) {
				/* out of ISRs */
				panic(KPANIC_APIC);
			}

			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + i + i;
			lo = *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) & LO_MASK;
			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + i + i + 1;
			hi = *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) & HI_MASK;

			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + i + i;
			 *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) = lo | isr | DLVRY_FIXED | DEST_PHYSCL | flags[i + ioapic->intstart];
			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + i + i + 1;
			 *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) = hi | (apic_getId() << 24);

			idt_installisr((struct IDTD* volatile)&IDT_BASE, isr, 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT);
		}
	}

	kfree(flags);
}

uint32_t apic_translateGsi(uint64_t code) {
	const uint32_t gsi = code & ISR_INTERNAL_MASK;
	if (gsi < 16) {
		return irq_remaps_inv[gsi];
	}
	return gsi;
}

void apic_maskIrq(uint32_t gsi) {
	kacquireMutex(ioapic_mutex);
	/* iterate IOAPICs to find the one in charge of the gsi */
	for (struct apic_ioapic* ioapic = ioapics; ioapic != 0; ioapic = ioapic->next) {
		if (gsi >= ioapic->mingsi && gsi < ioapic->maxgsi) {
			const uint32_t index = gsi - ioapic->mingsi;

			/* mask the irq */
			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + index + index;
			uint32_t lo = *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base);
			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + index + index + 1;
			uint32_t hi = *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base);

			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + index + index;
			 *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) = lo | MASK_INT;
			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + index + index + 1;
			 *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) = hi;
			 break;
		}
	}
	kreleaseMutex(ioapic_mutex);
}

#endif /* APIC_IOAPIC_C */
