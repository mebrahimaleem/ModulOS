/* interrupts.c - serial interrupts handler */
/* Copyright (C) 2026  Ebrahim Aleem
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

#include <stdint.h>
#include <stddef.h>

#include <serial/interrupts.h>
#include <serial/serial.h>

#include <kernel/core/idt.h>
#include <kernel/core/ports.h>

#include <kernel/ioapic/routing.h>
#include <kernel/ioapic/ioapic_init.h>

#include <kernel/apic/apic_regs.h>

#include <kernel/devfs/tty.h>

#define COM1_IRQ	4
#define COM2_IRQ	3

#define COM1	0x3F8
#define COM2	0x2F8

#define READ		0
#define LSR			5

#define DR_MASK	1

struct signal_wait_t* com1_signal;
struct signal_wait_t* com2_signal;

void serial_init_interrupts(void) {
	uint8_t com1_v = idt_get_vector();
	uint8_t com2_v = idt_get_vector();

	idt_install(com1_v, (uint64_t)serial_isr_com1, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);
	idt_install(com2_v, (uint64_t)serial_isr_com2, GDT_CODE_SEL, 0, IDT_GATE_TRP, 0);

	ioapic_conf_gsi(
			ioapic_routing_legacy_gsi(COM1_IRQ),
			com1_v,
			IOAPIC_REDIR_TRG_EDG | IOAPIC_REDIR_POL_HI,
			0);

	ioapic_conf_gsi(
			ioapic_routing_legacy_gsi(COM2_IRQ),
			com2_v,
			IOAPIC_REDIR_TRG_EDG | IOAPIC_REDIR_POL_HI,
			0);
}

void serial_isr_dispatch(uint16_t com) {
	struct tty_handle_t* tty;

	switch (com) {
		case COM1:
			tty = tty_com1();
			break;
		case COM2:
			tty = tty_com2();
			break;
		default:
			goto cleanup;
	}

	//TODO: handle serial errors
	while (inb(com + LSR) & DR_MASK) {
		tty_queue_read(tty, inb(com + READ));
		io_wait();
	}

cleanup:
	apic_write_reg(APIC_REG_EOI, APIC_EOI);
}
