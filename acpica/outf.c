/* outf.c - output formatting for ACPICA */
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

#ifndef ACPICA_OUTF_C
#define ACPICA_OUTF_C

#include <stdarg.h>

#include <core/utils.h>
#include <core/serial.h>

#include "acpi.h"

#include <acpica/outf.h>

void ACPI_INTERNAL_XFACE AcpiOsPrintf(const char *Format, ...) {
	va_list va;
	va_start(va, Format);
	AcpiOsVprintf(Format, va);
}

void AcpiOsVprintf(const char *Format, va_list Args) {
	serialVprintf(SERIAL1, Format, Args);
}

void AcpiOsRedirectOutput(void *Destination) {
	return;
}

#endif /* ACPICA_OUTF_C */
