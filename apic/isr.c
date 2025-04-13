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

#include <core/atomic.h>
#include <core/serial.h>
#include <core/idt.h>
#include <core/memory.h>

#include <apic/lapic.h>
#include <apic/ioapic.h>
#include <apic/isr.h>

struct ISR_callback* cached_callbacks[256];

mutex_t callbackLock;

void isr_init(void) {
	callbackLock = kcreateMutex();

	for (uint16_t i = 0; i < 256; i++)
		cached_callbacks[i] = 0;
}

void isr_registerCallback(void (*callback)(uint64_t), uint8_t v) {
	kacquireMutex(callbackLock);
	struct ISR_callback* cached = kmalloc(sizeof(struct ISR_callback));
	cached->callback = callback;
	cached->next = cached_callbacks[v];
	cached_callbacks[v] = cached;
	kreleaseMutex(callbackLock);
}


void isr_handler(uint64_t code) {
	/* check for spurious */
	if (code == LAPIC_SPURIOUS_VECTOR) {
		return;
	}

	uint64_t internal = idt_translateCode(code);

	/* handle pre eoi isrs */
	switch (internal) {
		case ISR_LAPIC_START + LAPIC_IPI_FLUSH_CODE: /* tlb flush */
			tlbflush();
			break;
		default:
			break;
	}

	// check if no eoi
	if (internal == ISR_LAPIC_START + LAPIC_LNT0_CODE) {
		return;
	}

	/* send eoi */
	apic_lapic_sendeoi();

	/* evaluate cached callbacks */
	kacquireMutex(callbackLock);
	for (struct ISR_callback* callback = cached_callbacks[code]; callback != 0; callback = callback->next) {
		callback->callback(internal);
	}

	kreleaseMutex(callbackLock);
}

#endif /* APIC_ISR_C */
