/* isr.g - interrupt service routines */
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

#ifndef APIC_ISR_H
#define APIC_ISR_H

#define ISR_INTERNAL_MASK	0x0FFF
#define ISR_LAPIC_START		0x1000
#define ISR_IOAPIC_START	0x2000

#include <core/scheduler.h>

#define V_TO_CODE(v) (v - 0x20)

struct ISR_callback {
	void (*callback)(uint64_t);
	struct ISR_callback* next;
};

void isr_init(void);

void isr_registerCallback(void (*callback)(uint64_t), uint8_t v);

void isr_handler(uint64_t code);

#endif /* APIC_ISR_H */
