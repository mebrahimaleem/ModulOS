/* ipi.c - Inter Processor Interrupt interface */
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

#ifndef KERNEL_APIC_IPI_H
#define KERNEL_APIC_IPI_H

#include <stdint.h>

#define AP_ENTRY_PAGE	8

extern void apic_init_shootdowns(uint8_t num_apic);
extern void apic_ipi_init(void);

extern void apic_wait_for_ipi(void);
extern void apic_send_ipi_init_set(uint8_t apic_id);
extern void apic_send_ipi_init_clear(uint8_t apic_id);
extern void apic_send_ipi_sipi(uint8_t apic_id);

extern void apic_tlb_shootdown(uint64_t vaddr);
extern void apic_tlb_shootdown_dispatch(void);

extern void apic_register_barrier(uint8_t apic_id);

#endif /* KERNEL_APIC_IPI_H */
