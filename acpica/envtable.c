/* envtable.c - environment and tables for ACPICA */
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

#ifndef ACPICA_ENVTABLE_C
#define ACPICA_ENVTABLE_C

#include "acpi.h"

#include <acpica/envtable.h>

ACPI_STATUS AcpiOsInitialize() {
	return AE_ERROR;
}

ACPI_STATUS AcpiOsTerminate() {
	return AE_ERROR;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer() {
	return 0;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue) {
	return AE_ERROR;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable) {
	return AE_ERROR;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength) {
	return AE_ERROR;
}

ACPI_STATUS AcpiOsInitializeDebugger() {
	return AE_ERROR;
}

void AcpiOsTerminateDebugger() {
	return;
}

#endif /* ACPICA_ENVTABLE_C */
