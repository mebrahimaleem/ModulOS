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
#include <kernel/core/alloc.h>
#include <kernel/core/cpu_instr.h>
#include <kernel/lib/kmemset.h>

#define IOAPIC_OFF_IOREGSEL	0x00
#define IOAPIC_OFF_IOWIN		0x10

#define IOAPIC_IOAPICVER	0x01
#define IOAPIC_REDIR_ST		0x10

#define MAX_REDIR_ENTRY_SHF	16
#define MAX_REDIR_ENTRY_MSK	0xFF

#define REDIR_DST_SHF				24

#define REDIR_NMI	0x500

struct redirection_entry_ptr_t {
	uint64_t ioapic_base;
	uint64_t gsi_base;
};

static struct redirection_entry_ptr_t* redir_index;

extern uint8_t bsp_apic_id;

void ioapic_init(void) {
	uint64_t gsi_max = 0;
	uint64_t handle;
	struct acpi_madt_ics_io_apic_t* ioapic;

	acpi_parse_madt_ics_start(&handle);
	for (acpi_parse_madt_ics((void*)&ioapic, &handle, MADT_ICS_IO_APIC);
			handle != 0;
			acpi_parse_madt_ics((void*)&ioapic, &handle, MADT_ICS_IO_APIC)) {

		logging_log_info("Initializing IO APIC 0x%X64", (uint64_t)ioapic->IOAPICID);
		const uint64_t ioapic_base = ioapic->IOAPICAddress;
		paging_map(ioapic_base, ioapic_base, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);

		*(volatile uint32_t*)(ioapic_base + IOAPIC_OFF_IOREGSEL) = IOAPIC_IOAPICVER;
		const uint64_t num_redir = 1 + (MAX_REDIR_ENTRY_MSK &
			(*(volatile uint32_t*)(ioapic_base + IOAPIC_OFF_IOWIN) >> MAX_REDIR_ENTRY_SHF));

		const uint64_t lcl_max_gsi = 1 + ioapic->GlobalSystemInterruptBase + num_redir;

		if (lcl_max_gsi > gsi_max) {
			gsi_max = lcl_max_gsi;
		}

		// unlikely, but map extra pages if needed
		for (uint64_t mapping_base = ioapic_base + PAGE_SIZE_4K;
				mapping_base < ioapic_base + IOAPIC_REDIR_ST + num_redir * 2;
				mapping_base += PAGE_SIZE_4K) {
			paging_map(mapping_base, mapping_base, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
		}
	}

	// map all gsis back to ioapics (second pass after knowing gsi_max)
	redir_index = kmalloc(sizeof(struct redirection_entry_ptr_t) * gsi_max);
	kmemset(redir_index, 0, sizeof(struct redirection_entry_ptr_t) * gsi_max);

	acpi_parse_madt_ics_start(&handle);
	for (acpi_parse_madt_ics((void*)&ioapic, &handle, MADT_ICS_IO_APIC);
			handle != 0;
			acpi_parse_madt_ics((void*)&ioapic, &handle, MADT_ICS_IO_APIC)) {

		const uint64_t ioapic_base = ioapic->IOAPICAddress;

		*(volatile uint32_t*)(ioapic_base + IOAPIC_OFF_IOREGSEL) = IOAPIC_IOAPICVER;
		const uint64_t num_redir = 1 + (MAX_REDIR_ENTRY_MSK &
			(*(volatile uint32_t*)(ioapic_base + IOAPIC_OFF_IOWIN) >> MAX_REDIR_ENTRY_SHF));

		for (uint64_t i = 0; i < num_redir; i++) {
			redir_index[i].ioapic_base = ioapic_base;
			redir_index[i].gsi_base = ioapic->GlobalSystemInterruptBase;
		}
	}
	
	// mask everything until needed
	for (uint64_t i = 0; i < gsi_max; i++) {
		ioapic_conf_gsi(i, 0, IOAPIC_REDIR_MASK, 0);
	}

	// set nmis
	struct acpi_madt_ics_nmi_source_t* nmi_source;
	acpi_parse_madt_ics_start(&handle);
	for (acpi_parse_madt_ics((void*)&nmi_source, &handle, MADT_ICS_NMI_SOURCE);
			handle != 0;
			acpi_parse_madt_ics((void*)&ioapic, &handle, MADT_ICS_NMI_SOURCE)) {
		logging_log_info("Found IO APIC NMI source on GSI 0x%X64", (uint64_t)nmi_source->GlobalSystemInterrupt);
		// guaranteed ISA, so we can assume conform is EDG and HI
		const uint32_t flg = IOAPIC_REDIR_TRG_EDG | REDIR_NMI |
			(((nmi_source->Flags.PolarityAndTriggerMode & MADT_ICS_MPS_POLARITY_MASK)
				== MADT_ICS_MPS_POLARITY_LO) ? IOAPIC_REDIR_POL_LO : IOAPIC_REDIR_POL_HI);
		ioapic_conf_gsi(nmi_source->GlobalSystemInterrupt, 0, flg, bsp_apic_id);
	}

	// initialize interrupt routing (also sets up ISA IRQs)
	ioapic_routing_init(gsi_max);

	// at this point minimum isrs are installed, everything else is masked
	logging_log_debug("Setting interrupt flag");
	cpu_sti();
}

void ioapic_conf_gsi(uint64_t gsi, uint8_t v, uint32_t flg, uint8_t dest) {
	const uint32_t redir = (uint32_t)(gsi - redir_index[gsi].gsi_base);

	*(volatile uint32_t*)(redir_index[gsi].ioapic_base + IOAPIC_OFF_IOREGSEL) = 
		IOAPIC_REDIR_ST + 2 * redir;
	*(volatile uint32_t*)(redir_index[gsi].ioapic_base + IOAPIC_OFF_IOWIN) =
		(uint32_t)v | flg;

	*(volatile uint32_t*)(redir_index[gsi].ioapic_base + IOAPIC_OFF_IOREGSEL) = 
		IOAPIC_REDIR_ST + 2 * redir + 1;
	*(volatile uint32_t*)(redir_index[gsi].ioapic_base + IOAPIC_OFF_IOWIN) =
		(uint32_t)dest << REDIR_DST_SHF;
}
