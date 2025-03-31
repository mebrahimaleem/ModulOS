/* tables.c - system table access for ACPICA */
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

#ifndef ACPICA_TABLES_C
#define ACPICA_TABLES_C

#include "acpi.h"

#include <acpica/tables.h>

ACPI_STATUS AcpiOsGetTableByAddress(ACPI_PHYSICAL_ADDRESS Address, ACPI_TABLE_HEADER **OutTable) {
	return AE_ERROR;
}

ACPI_STATUS AcpiOsGetTableByIndex(UINT32 TableIndex, ACPI_TABLE_HEADER **OutTable, UINT32 *Instance, ACPI_PHYSICAL_ADDRESS *OutAddress) {
	return AE_ERROR;
}

ACPI_STATUS AcpiOsGetTableByName(char *Signature, UINT32 Instance, ACPI_TABLE_HEADER **OutTable, ACPI_PHYSICAL_ADDRESS *OutAddress) {
	return AE_ERROR;
}

#endif /* ACPICA_TABLES_C */
