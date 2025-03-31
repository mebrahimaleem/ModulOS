/* pic8259.c - 8259 PIC routines */
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

#ifndef APIC_PIC8259_C
#define APIC_PIC8259_C

#include <apic/pic8259.h>

#include <core/portio.h>

#define PIC1_DATA	0x20 + 1
#define PIC2_DATA	0xA0 + 1

#define PIC_MASK	0xFF

void pic_mask8259(void) {
	outb(PIC1_DATA, PIC_MASK);
	outb(PIC2_DATA, PIC_MASK);
	iowait();
}

#endif /* APIC_PIC8259_C */
