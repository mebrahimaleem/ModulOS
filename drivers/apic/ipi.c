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

#include <apic/ipi.h>
#include <apic/apic_regs.h>

#include <kernel/core/cpu_instr.h>

#define ICR_LEVEL			0x8000
#define ICR_ASSERT		0x4000
#define ICR_DS				0x1000
#define ICR_LO_INIT		0x0500
#define ICR_LO_SIPI		(0x0600 | AP_ENTRY_PAGE)

#define ICR_PID_SHFT	24

void apic_wait_for_ipi(void) {
	while (apic_read_reg(APIC_REG_ICL) & ICR_DS) {
		cpu_pause();
	}
}

void apic_send_ipi_init_set(uint8_t apic_id) {
	apic_write_reg(APIC_REG_ICH, (uint32_t)apic_id << ICR_PID_SHFT);
	apic_write_reg(APIC_REG_ICL, ICR_LO_INIT | ICR_LEVEL | ICR_ASSERT );
}

void apic_send_ipi_init_clear(uint8_t apic_id) {
	apic_write_reg(APIC_REG_ICH, (uint32_t)apic_id << ICR_PID_SHFT);
	apic_write_reg(APIC_REG_ICL, ICR_LO_INIT | ICR_LEVEL);
}

void apic_send_ipi_sipi(uint8_t apic_id) {
	apic_write_reg(APIC_REG_ICH, (uint32_t)apic_id << ICR_PID_SHFT);
	apic_write_reg(APIC_REG_ICL, ICR_LO_SIPI);
}
