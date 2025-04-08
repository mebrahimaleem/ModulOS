/* madt.c - ACPI MADT Structure */
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

#ifndef ACPI_MADT_C
#define ACPI_MADT_C

#include <stdint.h>

#include <core/panic.h>

#include <acpi/acpica.h>
#include <acpi/madt.h>

#define PCAT_COMPAT	1

uint8_t acpi_needDisable8259() {
	struct acpi_madt* madt = acpi_getMadt();
	if (madt == 0) {
		panic(KPANIC_ACPI);
	}

	return (madt->flg & PCAT_COMPAT) == PCAT_COMPAT;
}

uint64_t acpi_nextMADT(uint8_t type, uint64_t hint, struct acpi_madt_ic** madtPtr) {
	struct acpi_madt* madt = acpi_getMadt();

	struct acpi_madt_ic* ics = (struct acpi_madt_ic*)((uint64_t)&madt->structs[0] + hint);

	while ((uint64_t)ics < (uint64_t)madt + madt->len) {
		if (ics->type == type) {
			*madtPtr = ics;

			return (uint64_t)ics - (uint64_t)&madt->structs[0] + ics->len;
		}
		
		ics = (struct acpi_madt_ic*)((uint64_t)ics + ics->len);
	}

	return 0;
}

#endif /* ACPI_MADT_C */
