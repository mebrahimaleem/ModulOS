/* madt.h - ACPI MADT Structure */
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

#ifndef ACPI_MADT_H
#define ACPI_MADT_H

#include <stdint.h>

#define MADT_LAPIC_TYPE		0x0
#define MADT_IOAPIC_TYPE	0x1
#define MADT_REDIR_TYPE		0x2

struct acpi_madt_ic {
	uint8_t type;
	uint8_t len;
} __attribute__((packed));

struct acpi_lapic {
	uint8_t type;
	uint8_t len;
	uint8_t acpiId;
	uint8_t apicId;
	uint32_t enabled : 1;
	uint32_t capable : 1;
	uint32_t resv : 30;
} __attribute__((packed));

struct acpi_ioapic {
	uint8_t type;
	uint8_t len;
	uint8_t apicId;
	uint8_t resv;
	uint32_t base;
	uint32_t intstart;
} __attribute__((packed));

struct acpi_overrides {
	uint8_t type;
	uint8_t len;
	uint8_t bus;
	uint8_t source;
	uint32_t gsi;
	uint16_t polarity : 2;
	uint16_t trigger : 2;
	uint16_t resv : 12;
} __attribute__((packed));

struct acpi_madt {
	uint8_t sig[4];
	uint32_t len;
	uint8_t rev;
	uint8_t sum;
	uint8_t oemid[6];
	uint8_t oemtblid[8];
	uint32_t oemrev;
	uint32_t creatorid;
	uint32_t creatorrev;
	uint32_t lapicaddr;
	uint32_t flg;
	struct acpi_madt_ic structs[1];
} __attribute__((packed));

uint8_t acpi_needDisable8259(void);

uint64_t acpi_nextMADT(uint8_t type, uint64_t hint, struct acpi_madt_ic** madtPtr);

#endif /* ACPI_MADT_H */
