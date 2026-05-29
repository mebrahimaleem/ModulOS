/* ipi.c - Inter Processor Interrupt functions */
/* Copyright (C) 2025-2026  Ebrahim Aleem
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

#include <apic/ipi.h>
#include <apic/apic_regs.h>
#include <apic/isr.h>

#include <core/cpu_instr.h>
#include <core/lock.h>
#include <core/alloc.h>
#include <core/idt.h>
#include <core/proc_data.h>
#include <core/mm.h>
#include <core/paging.h>

#include <lib/kmemset.h>

#define ICR_LEVEL			0x8000u
#define ICR_ASSERT		0x4000u
#define ICR_DS				0x1000u
#define ICR_LO_INIT		0x0500u
#define ICR_LO_SIPI		(0x0600u | AP_ENTRY_PAGE)

#define ICR_PID_SHFT	24

static uint8_t tlb_shootdown_vector;
static uint8_t shootdown_enable = 0;

struct shootdown_node_t {
	struct shootdown_node_t* next;
	uint8_t apic_id;
};

static uint8_t ipi_lock;

void apic_ipi_init(void) {
	lock_init(&ipi_lock);
}

void apic_wait_for_ipi(void) {
	while (apic_read_reg(APIC_REG_ICL) & ICR_DS) {
		cpu_pause();
	}
}

void apic_send_ipi_init_set(uint8_t apic_id) {
	lock_acquire(&ipi_lock);
	apic_wait_for_ipi();
	apic_write_reg(APIC_REG_ICH, (uint32_t)apic_id << ICR_PID_SHFT);
	apic_write_reg(APIC_REG_ICL, ICR_LO_INIT | ICR_LEVEL | ICR_ASSERT );
	lock_release(&ipi_lock);
}

void apic_send_ipi_init_clear(uint8_t apic_id) {
	lock_acquire(&ipi_lock);
	apic_wait_for_ipi();
	apic_write_reg(APIC_REG_ICH, (uint32_t)apic_id << ICR_PID_SHFT);
	apic_write_reg(APIC_REG_ICL, ICR_LO_INIT | ICR_LEVEL);
	lock_release(&ipi_lock);
}

void apic_send_ipi_sipi(uint8_t apic_id) {
	lock_acquire(&ipi_lock);
	apic_wait_for_ipi();
	apic_write_reg(APIC_REG_ICH, (uint32_t)apic_id << ICR_PID_SHFT);
	apic_write_reg(APIC_REG_ICL, ICR_LO_SIPI);
	lock_release(&ipi_lock);
}

void apic_init_shootdowns(void) {
	tlb_shootdown_vector = idt_get_vector();

	idt_install(tlb_shootdown_vector, (uint64_t)apic_isr_tlb_shootdown, GDT_CODE_SEL, 0, IDT_GATE_INT, 0);

	shootdown_enable = 1;
}

void apic_shootdown(uint8_t id) {
	if (!shootdown_enable) {
		return;
	}

	lock_acquire(&ipi_lock);
	apic_wait_for_ipi();
	apic_wait_for_ipi();
	apic_write_reg(APIC_REG_ICH, (uint32_t)id << ICR_PID_SHFT);
	apic_write_reg(APIC_REG_ICL, tlb_shootdown_vector | ICR_ASSERT);
	lock_release(&ipi_lock);
}

void apic_tlb_shootdown_dispatch(void) {
	uint64_t off;
	struct free_transaction_list_t* list;

	for (list = mm_get_shootdown_list(); list; list = list->next) {
		for (off = 0; off < list->size; off += PAGE_SIZE_4K) {
			cpu_invlpg(list->base + off);
		}
	}

	mm_barrier_disarm(proc_data_get()->arb_id);

	apic_write_reg(APIC_REG_EOI, APIC_EOI);
}
