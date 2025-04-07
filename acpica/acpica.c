/* acpica.c - acpica exposed functions */
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

#ifndef ACPICA_ACPICA_C
#define ACPICA_ACPICA_C

#include "acpi.h"

#include <acpica/acpica.h>

uint8_t acpiinit() {
	ACPI_STATUS Status = AcpiInitializeSubsystem();

	if (ACPI_FAILURE(Status)) {
		return 1;
	}

	Status = AcpiInitializeTables(NULL, 16, FALSE);

	if (ACPI_FAILURE(Status)) {
		return 1;
	}

	Status = AcpiLoadTables();

	if (ACPI_FAILURE(Status)) {
		return 1;
	}

	return 0;
}

void* acpi_getMadt() {
	ACPI_TABLE_HEADER* Table;
	ACPI_STATUS Status = AcpiGetTable(ACPI_SIG_MADT, 1, &Table);

	if (ACPI_FAILURE(Status)) {
		return (void*)0;
	}

	return (void*)Table;
}

#endif /* ACPICA_ACPICA_C */
