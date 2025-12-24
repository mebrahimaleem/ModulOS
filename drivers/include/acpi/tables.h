/* tables.h - ACPI tables interface */
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

#ifndef DRIVERS_ACPI_TABLES_H
#define DRIVERS_ACPI_TABLES_H

#include <stdint.h>

#define MADT_ICS_PROCESSOR_LOCAL_APIC				0
#define MADT_ICS_IO_APIC										1
#define MADT_ICS_INTERRUPT_SOURCE_OVERRIDE	2
#define MADT_ICS_LOCAL_APIC_NMI							4

#define MADT_ICS_MPS_POLARITY_MASK					0x3
#define MADT_ICS_MPS_POLARITY_CONF					0x0
#define MADT_ICS_MPS_POLARITY_HI						0x1
#define MADT_ICS_MPS_POLARITY_LO						0x3
#define MADT_ICS_MPS_TRIGGER_MASK						0xC
#define MADT_ICS_MPS_TRIGGER_CONF						0x0
#define MADT_ICS_MPS_TRIGGER_EDGE						0x4
#define MADT_ICS_MPS_TRIGGER_LEVL						0xC

#define ACPI_UID_ALL_PROC	0xFF

struct acpi_rsdp_t {
	uint8_t		Signature[8];
	uint8_t		Checksum;
	uint8_t		OEMID[6];
	uint8_t		Revision;
	uint32_t	RsdtAddress;
	uint32_t  Length;
	uint64_t	XsdtAddress;
	uint8_t		ExtendedChecksum;
	uint8_t		Reserved[3];
} __attribute__((packed));

struct acpi_madt_ics_gen_t {
	uint8_t Type;
	uint8_t Length;
	uint8_t _[];
} __attribute__((packed));

struct acpi_madt_ics_mps_t {
	uint8_t PolarityAndTriggerMode;
	uint8_t Reserved;
} __attribute__((packed));

struct acpi_madt_ics_local_apic_t {
	uint8_t Type;
	uint8_t Length;
	uint8_t ACPIProcessorUID;
	uint8_t APICID;
	uint8_t Flags;
} __attribute__((packed));

struct acpi_madt_ics_io_apic_t {
	uint8_t Type;
	uint8_t Length;
	uint8_t IOAPICID;
	uint8_t Reserved;
	uint32_t IOAPICAddress;
	uint32_t GlobalSystemInterruptBase;
} __attribute__((packed));

struct acpi_madt_ics_interrupt_source_override_t {
	uint8_t Type;
	uint8_t Length;
	uint8_t Bus;
	uint8_t Source;
	uint32_t GlobalSystemInterrupt;
	struct acpi_madt_ics_mps_t Flags;
} __attribute__((packed));

struct acpi_madt_ics_local_apic_nmi_t {
	uint8_t Type;
	uint8_t Length;
	uint8_t ACPIProcessorUID;
	struct acpi_madt_ics_mps_t Flags;
	uint8_t LocalAPICLINTNum;
} __attribute__((packed));

extern void acpi_copy_tables(void);

extern void acpi_parse_madt_ics_start(uint64_t* handle);
extern void acpi_parse_madt_ics(struct acpi_madt_ics_gen_t** ics, uint64_t* handle, uint8_t type);

#endif /* DRIVERS_ACPI_TABLES_H */
