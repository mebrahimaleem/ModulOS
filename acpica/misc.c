/* misc.c - Miscellaneous for ACPICA */
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

#ifndef ACPICA_MISC_C
#define ACPICA_MISC_C

#include "acpi.h"

#include <acpica/misc.h>

UINT64 AcpiOsGetTimer() {
	return 0;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
	return AE_ERROR;
}

ACPI_STATUS AcpiOsGetLine(char *Buffer, UINT32 BufferLength, UINT32 *BytesRead) {
	return AE_ERROR;
}

#endif /* ACPICA_MISC_C */
