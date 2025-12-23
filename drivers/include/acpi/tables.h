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

extern void acpi_index_tables(void);

#endif /* DRIVERS_ACPI_TABLES_H */
