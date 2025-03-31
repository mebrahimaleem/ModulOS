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

struct acpi_madt_ic {
	uint8_t type;
	uint8_t len;
	void* unspec[1];
} __attribute__((packed));

struct acpi_madt {
	uint8_t sig[4];
	uint64_t len;
	uint8_t rev;
	uint8_t sum;
	uint8_t oemid[6];
	uint8_t oemtblid[8];
	uint64_t oemrev;
	uint64_t creatorid;
	uint64_t creatorrev;
	uint64_t lapicaddr;
	uint64_t flg;
	struct acpi_madt_ic structs[1];
} __attribute__((packed));

uint8_t acpi_needDisable8259(void);

#endif /* ACPI_MADT_H */
