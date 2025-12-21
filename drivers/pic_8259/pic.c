/* pic.c - prgrammable interrupt controller driver */
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

#include "pic_8259/isr.h"
#include <pic_8259/pic.h>

#include <kernel/core/ports.h>
#include <kernel/core/idt.h>

#define PIC1_CMD	0x20
#define PIC1_DATA	0x21

#define PIC2_CMD	0xA0
#define	PIC2_DATA	0xA1

#define ICW1_INIT	0x10
#define ICW1_ICW4	0x01
#define ICW4_8086	0x01

#define CASCADE_BIT	0x4
#define PIC2_ID			0x2

#define PIC1_IRQ_BASE	0x20
#define PIC2_IRQ_BASE	0x28
#define PIC_SPURIOUS	0x7

void pic_disab(void) {
	outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4); // cascade sequence init
	io_wait();
	outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
	io_wait();

	outb(PIC1_DATA, PIC1_IRQ_BASE);
	io_wait();
	outb(PIC2_DATA, PIC2_IRQ_BASE);
	io_wait();

	outb(PIC1_DATA, CASCADE_BIT);
	io_wait();
	outb(PIC2_DATA, PIC2_ID);
	io_wait();

	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	idt_install(PIC1_IRQ_BASE + PIC_SPURIOUS, (uint64_t)pic_spurious_master, GDT_CODE_SEL, 0, IDT_GATE_INT, 0);
	idt_install(PIC2_IRQ_BASE + PIC_SPURIOUS, (uint64_t)pic_spurious_slave, GDT_CODE_SEL, 0, IDT_GATE_INT, 0);

	// mask
	outb(PIC1_DATA, 0xff);
	io_wait();
	outb(PIC2_DATA, 0xff);
	io_wait();
}
