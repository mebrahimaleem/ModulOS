/* routing.c - IO/APIC interrupt routing */
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

#include "ioapic/ioapic_init.h"
#include <stdint.h>

#include <ioapic/routing.h>

#include <acpi/tables.h>

#include <kernel/core/alloc.h>
#include <kernel/core/logging.h>
#include <kernel/lib/kmemset.h>

#define NUM_LEGACY	16

struct gsi_route_t {
	const char* purpose;
	enum {
		GSI_TYPE_UNUSED,
		GSI_TYPE_LEGACY
	} type;
	union {
		uint64_t isa_src;
	} meta;
};

static const char* legacy_purpose[NUM_LEGACY] = {
	"ISA PIT", "ISA KBD", "ISA Cascade", "ISA COM2", "ISA COM1", "ISA LPT2", "ISA Floppy", "ISA LPT1",
	"ISA CMOS RTC", "ISA PER", "ISA PER", "ISA PER", "ISA Mouse", "ISA CO-PROC", "ISA ATA1", "ISA ATA2"
};

static uint64_t legacy_routing[NUM_LEGACY];

static struct gsi_route_t* gsi_routing;

static inline uint8_t gsi_is_identity_isa(uint64_t gsi) {
	return gsi_routing[gsi].type == GSI_TYPE_LEGACY &&
		gsi_routing[gsi].meta.isa_src == gsi;
}

void ioapic_routing_init(uint64_t num_gsi) {
	gsi_routing = kmalloc(num_gsi * sizeof(struct gsi_route_t));

	for (uint64_t i = 0; i < num_gsi; i++) {
		gsi_routing[i].purpose = "Unused";
		gsi_routing[i].type = GSI_TYPE_UNUSED;
		legacy_routing[i] = i;
	}

	// map legacy
	for (uint8_t i = 0; i < NUM_LEGACY; i++) {
		gsi_routing[i].purpose = legacy_purpose[i];
		gsi_routing[i].type = GSI_TYPE_LEGACY;
		gsi_routing[i].meta.isa_src = i;
	}

	uint64_t handle;
	struct acpi_madt_ics_interrupt_source_override_t* override;
	acpi_parse_madt_ics_start(&handle);
	for (acpi_parse_madt_ics((void*)&override, &handle, MADT_ICS_INTERRUPT_SOURCE_OVERRIDE);
			handle != 0;
			acpi_parse_madt_ics((void*)&override, &handle, MADT_ICS_INTERRUPT_SOURCE_OVERRIDE)) {

		logging_log_debug("GSI 0x%X64 -> ISA IRQ 0x%X64, 0x%X64 (pol) 0x%X64 (trg)",
				(uint64_t)override->GlobalSystemInterrupt,
				(uint64_t)override->Source,
				(uint64_t)(override->Flags.PolarityAndTriggerMode & MADT_ICS_MPS_POLARITY_MASK),
				(uint64_t)(override->Flags.PolarityAndTriggerMode & MADT_ICS_MPS_TRIGGER_MASK) >> 2);

		// almost guaranteed ISA, so we can assume conform is EDG and HI
		const uint32_t flg =
			(((override->Flags.PolarityAndTriggerMode & MADT_ICS_MPS_TRIGGER_MASK)
				== MADT_ICS_MPS_TRIGGER_LEVL) ? IOAPIC_REDIR_TRG_LVL : IOAPIC_REDIR_TRG_EDG) |
			(((override->Flags.PolarityAndTriggerMode & MADT_ICS_MPS_POLARITY_MASK)
				== MADT_ICS_MPS_POLARITY_LO) ? IOAPIC_REDIR_POL_LO : IOAPIC_REDIR_POL_HI) |
			IOAPIC_REDIR_MASK; // leave masked until we need it

		ioapic_conf_gsi(override->GlobalSystemInterrupt, 0, flg, 0);
		// update routing
		gsi_routing[override->GlobalSystemInterrupt].purpose = legacy_purpose[override->Source];
		gsi_routing[override->GlobalSystemInterrupt].type = GSI_TYPE_LEGACY;
		legacy_routing[override->Source] = override->GlobalSystemInterrupt;
		// clear old conditionally (in case already overriden)
		if (gsi_is_identity_isa(override->GlobalSystemInterrupt)) {
			gsi_routing[override->Source].purpose = "Unused";
			gsi_routing[override->Source].type = GSI_TYPE_UNUSED;
		}
	}

	// identity map everything set to identity ISA
	for (uint8_t i = 0; i < NUM_LEGACY; i++) {
		if (gsi_is_identity_isa(i)) {
			ioapic_conf_gsi(i, 0, IOAPIC_REDIR_TRG_EDG | IOAPIC_REDIR_POL_HI | IOAPIC_REDIR_MASK, 0);
		}
	}
}

uint64_t ioapic_routing_legacy_gsi(uint8_t isa_irq) {
	return legacy_routing[isa_irq];
}
