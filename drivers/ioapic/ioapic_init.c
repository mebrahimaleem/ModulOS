/* ioapic_init.c - IO/APIC interface */
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

#include <ioapic/ioapic_init.h>
#include <ioapic/routing.h>

#include <acpi/tables.h>

#include <kernel/core/paging.h>
#include <kernel/core/logging.h>

#define IOAPIC_OFF_IOREGSEL	0x00
#define IOAPIC_OFF_IOWIN		0x10

#define IOAPIC_IOAPICVER	0x01

#define MAX_REDIR_ENTRY_SHF	16
#define MAX_REDIR_ENTRY_MSK	0xFF

void ioapic_init(void) {
	uint64_t gsi_max = 0;
	uint64_t ioapic_base;
	uint64_t handle;
	struct acpi_madt_ics_io_apic_t* ioapic;

	acpi_parse_madt_ics_start(&handle);
	for (acpi_parse_madt_ics((void*)&ioapic, &handle, MADT_ICS_IO_APIC);
			handle != 0;
			acpi_parse_madt_ics((void*)&ioapic, &handle, MADT_ICS_IO_APIC)) {

		logging_log_info("Initializing IO APIC 0x%X64", (uint64_t)ioapic->IOAPICID);
		ioapic_base = ioapic->IOAPICAddress;
		paging_map(ioapic_base, ioapic_base, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);

		*(volatile uint32_t*)(ioapic_base + IOAPIC_OFF_IOREGSEL) = IOAPIC_IOAPICVER;
		const uint64_t lcl_max_gsi = 1 + ioapic->GlobalSystemInterruptBase + (MAX_REDIR_ENTRY_MSK &
			(*(volatile uint32_t*)(ioapic_base + IOAPIC_OFF_IOWIN) >> MAX_REDIR_ENTRY_SHF));

		if (lcl_max_gsi > gsi_max) {
			gsi_max = lcl_max_gsi;
		}
	}

	ioapic_routing_init(gsi_max);
}
