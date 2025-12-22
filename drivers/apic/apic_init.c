/* apic_init.h - Advanced programmable interrupt controller initialization interface */
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

#include <stdint.h>


#include <apic/apic_init.h>
#include <apic/apic_regs.h>
#include <apic/isr.h>
#include <pic_8259/pic.h>

#include <kernel/core/logging.h>
#include <kernel/core/msr.h>
#include <kernel/core/paging.h>
#include <kernel/core/mm.h>
#include <kernel/core/ports.h>
#include <kernel/core/idt.h>
#include <kernel/core/cpu_instr.h>

#define APIC_BASE_MASK	0xFFFFFFFFFF000

#define APIC_ID_SHFT	24
#define APIC_VER_EAS	0x80000000
#define APIC_XLC_SHFT	16
#define APIC_XLC_MASK	8

#define APIC_LVT_MT_FIXED	0x0
#define APIC_LVT_MT_SMI		0x2
#define APIC_LVT_MT_NMI		0x4
#define APIC_LVT_MT_EXT		0x7

#define APIC_LVT_TRG_EDGE	0x00
#define APIC_LVT_TRG_LEVL	0x80

#define APIC_LVT_MASK			0x01
#define APIC_LVT_TMR_PER	0x02
#define APIC_LVT_TMR_ONE	0x00

// pic master spurious (irq 7, int 0x27) works for apic spurious as well
#define PIC_SPURIOUS_VEC	0x27
#define APIC_ASE					0x100

#define CMOS_RAM_INDEX	0x70
#define CMOS_RAM_DATA		0x71
#define NMI_DISAB_MASK	0x7F

#define APIC_TMR_DIV_1	0xB

uint64_t apic_base;

void apic_init(void) {
	pic_disab();

	apic_base = mm_alloc_dv(MM_ORDER_4K);
	paging_map(apic_base, msr_read(MSR_APIC_BASE) & APIC_BASE_MASK,
			PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);

	logging_log_info("Initializing Local APIC 0x%X64",
			(uint64_t)(apic_read_reg(APIC_REG_IDR) >> APIC_ID_SHFT));

	// mask all interrupts for now
	
	// check eas, if set then mask ext lvt based on xlc
	if ((apic_read_reg(APIC_REG_VER) & APIC_VER_EAS) == APIC_VER_EAS) {
		const uint8_t xlc = (apic_read_reg(APIC_REG_EFR) >> APIC_XLC_SHFT) & APIC_XLC_MASK;
		logging_log_debug("Found %u64 extended lvt registers", (uint64_t)xlc);
		if (xlc >= 1) {
			apic_write_lve(APIC_REG_EE0, 0, 0, APIC_LVT_MASK);
		}
		if (xlc >= 2) {
			apic_write_lve(APIC_REG_EE1, 0, 0, APIC_LVT_MASK);
		}
		if (xlc >= 3) {
			apic_write_lve(APIC_REG_EE2, 0, 0, APIC_LVT_MASK);
		}
		if (xlc >= 4) {
			apic_write_lve(APIC_REG_EE3, 0, 0, APIC_LVT_MASK);
		}
	}

	const uint8_t timer_vector = idt_get_vector();
	const uint8_t error_vector = idt_get_vector();

	// set LVT entries
	apic_write_lve(APIC_REG_TME, timer_vector,
			APIC_LVT_MT_FIXED | APIC_LVT_TRG_EDGE, APIC_LVT_TMR_ONE); // oneshot for init

	apic_write_lve(APIC_REG_THE, 0,
			0, APIC_LVT_MASK);

	apic_write_lve(APIC_REG_PRE, 0,
			0, APIC_LVT_MASK);

	apic_write_lve(APIC_REG_L0E, 0,
			APIC_LVT_MT_EXT | APIC_LVT_TRG_EDGE, 0); // pic masked and litm clear

	apic_write_lve(APIC_REG_L1E, 0,
			APIC_LVT_MT_NMI | APIC_LVT_TRG_EDGE, 0); // nmi, v is ignored

	apic_write_lve(APIC_REG_ERE, error_vector,
			APIC_LVT_MT_FIXED | APIC_LVT_TRG_EDGE, 0);

	// install idts, init timer first
	idt_install(timer_vector, (uint64_t)apic_isr_timer_init, GDT_CODE_SEL, 0, IDT_GATE_INT, 0);
	idt_install(error_vector, (uint64_t)apic_isr_error, GDT_CODE_SEL, 0, IDT_GATE_INT, 0);

	// enable apic
	apic_write_reg(APIC_REG_ESR, 0);
	apic_write_reg(APIC_REG_SPR, PIC_SPURIOUS_VEC | APIC_ASE); // pic and apic spurious both only iret, so reuse

	logging_log_debug("Setting interrupt flag");
	cpu_sti();

	// calibrate APIC timer
	// TODO
}

void apic_nmi_enab(void) {
	outb(CMOS_RAM_INDEX, inb(CMOS_RAM_INDEX) & NMI_DISAB_MASK);
	io_wait();
	inb(CMOS_RAM_DATA);
}
