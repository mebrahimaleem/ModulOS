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
#include <core/serial.h>
#include <core/memory.h>

#include <acpi/madt.h>

#include <apic/lapic.h>
#include <apic/ioapic.h>

#define IOREGSEL_OFF	0x00
#define IOWIN_OFF			0x10

#define IOAPICVER			0x01
#define IOREDTBL_ST		(uint32_t)0x10

#define LO_MASK				0xFFFE5000
#define HI_MASK				0x00FFFFFF

#define DLVRY_FIXED		0x0000
#define DEST_PHYSCL		0x0000
#define INTOL_LOWA		0x2000
#define TRIG_LEVEL		0x8000

#define OVER_LOA			0x3
#define OVER_TRL			0x3

uint8_t irq_remaps[240];

void apic_initio() {
	uint64_t hint = 0;
	uint8_t i;

	/* first iterate through source overrides to find correct config */
	uint32_t* flags = kcalloc(240, 4);
	for (i = 16; i < 240; i++) {
		flags[i] = INTOL_LOWA | TRIG_LEVEL;
		irq_remaps[i] = i; //default to identity
	}

	struct acpi_overrides* overrides;
	while (1) {
		hint = acpi_nextMADT(MADT_REDIR_TYPE, hint, (struct acpi_madt_ic**)&overrides);
		
		if (hint == 0) {
			break;
		}

		flags[overrides->gsi] = 0;
		irq_remaps[overrides->gsi] = overrides->source;

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
	uint16_t isr;

	const uint64_t irq_size = (uint64_t)&irq_dummy_end - (uint64_t)&irq_dummy_end;
	while (1) {
		hint = acpi_nextMADT(MADT_IOAPIC_TYPE, hint, (struct acpi_madt_ic**)&ioapic);
		
		if (hint == 0) {
			break;
		}

		*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOAPICVER;
		const uint8_t maxreds = (uint8_t)(*(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) >> 16);
		
		for (i = 0; i <= maxreds; i++) {
			isr = idt_getIsrVector();

			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + i + i;
			lo = *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) & LO_MASK;
			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + i + i + 1;
			hi = *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) & HI_MASK;

			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + i + i;
			 *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) = lo | isr | DLVRY_FIXED | DEST_PHYSCL | flags[i + ioapic->intstart];
			*(uint32_t* volatile)(uint64_t)(IOREGSEL_OFF + ioapic->base) = IOREDTBL_ST + i + i + 1;
			 *(uint32_t* volatile)(uint64_t)(IOWIN_OFF + ioapic->base) = hi | (apic_getId() << 24);

			idt_installisr((struct IDTD* volatile)&IDT_BASE, (uint64_t)&irq_handlers + irq_size * (isr - 0x27), 0, IDT_TYPE_INT, IDT_KDPL, IDT_PRESENT, (uint8_t)isr);
		}
	}

	kfree(flags);
}

void apic_ioapic_ISRHandler(uint64_t code) {
	//TODO: implement	

	apic_lapic_sendeoi();
}
#endif /* APIC_IOAPIC_C */
