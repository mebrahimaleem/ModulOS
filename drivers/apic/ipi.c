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

#include <kernel/core/cpu_instr.h>
#include <kernel/core/lock.h>
#include <kernel/core/alloc.h>
#include <kernel/core/idt.h>
#include <kernel/core/proc_data.h>

#include <kernel/lib/kmemset.h>

#define ICR_LEVEL			0x8000u
#define ICR_ASSERT		0x4000u
#define ICR_DS				0x1000u
#define ICR_LO_INIT		0x0500u
#define ICR_LO_SIPI		(0x0600u | AP_ENTRY_PAGE)

#define ICR_PID_SHFT	24

static uint8_t tlb_shootdown_lock;
static uint8_t* registered_barrier;
static volatile uint8_t* tlb_shootdown_barrier = 0;
static uint8_t tlb_shootdown_vector;
static uint8_t barrier_len;
static volatile uint64_t tlb_shootdown_addr;

struct shootdown_node_t {
	struct shootdown_node_t* next;
	uint8_t apic_id;
};

static struct shootdown_node_t* shootdown_list;

static uint8_t ipi_lock;

void apic_ipi_init(void) {
	lock_init(&ipi_lock);
}

void apic_init_shootdowns(uint8_t num_apic) {
	lock_init(&tlb_shootdown_lock);

	registered_barrier = kmalloc(sizeof(uint8_t) * num_apic);

	kmemset(registered_barrier, 0, sizeof(uint8_t) * num_apic);
	shootdown_list = 0;

	tlb_shootdown_barrier = kmalloc(sizeof(uint8_t) * num_apic);
	barrier_len = num_apic;

	tlb_shootdown_vector = idt_get_vector();

	idt_install(tlb_shootdown_vector, (uint64_t)apic_isr_tlb_shootdown, GDT_CODE_SEL, 0, IDT_GATE_INT, 0);
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

static inline uint8_t check_barrier(volatile uint8_t* barrier) {
	uint8_t i;
	for (i = 0; i < barrier_len; i++) {
		if (barrier[i]) {
			return 1;
		}
	}

	return 0;
}

void apic_tlb_shootdown(uint64_t vaddr) {
	struct shootdown_node_t* node;
	if (!tlb_shootdown_barrier) {
		return;
	}

	lock_acquire(&tlb_shootdown_lock);

	uint8_t i;
	for (i = 0; i < barrier_len; i++) {
		tlb_shootdown_barrier[i] = registered_barrier[i];
	}

	tlb_shootdown_addr = vaddr;


	for (node = shootdown_list; node; node = node->next) {
		lock_acquire(&ipi_lock);
		apic_wait_for_ipi();
		apic_wait_for_ipi();
		apic_write_reg(APIC_REG_ICH, (uint32_t)node->apic_id << ICR_PID_SHFT);
		apic_write_reg(APIC_REG_ICL, tlb_shootdown_vector | ICR_ASSERT);
		lock_release(&ipi_lock);
	}

	while (check_barrier(tlb_shootdown_barrier)) cpu_pause();

	lock_release(&tlb_shootdown_lock);
}

void apic_tlb_shootdown_dispatch(void) {
	cpu_invlpg(tlb_shootdown_addr);

	tlb_shootdown_barrier[proc_data_get()->arb_id] = 0;

	apic_write_reg(APIC_REG_EOI, APIC_EOI);
}

void apic_register_barrier(uint8_t apic_id) {
	struct shootdown_node_t* node = kmalloc(sizeof(struct shootdown_node_t));
	node->apic_id = apic_id;

	lock_acquire(&tlb_shootdown_lock);

	node->next = shootdown_list;
	shootdown_list = node;
	registered_barrier[proc_data_get()->arb_id] = 1;

	lock_release(&tlb_shootdown_lock);
}
