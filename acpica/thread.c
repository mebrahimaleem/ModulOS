/* thread.c - threading and scheduling services for ACPICA */
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

#ifndef ACPICA_THREAD_C
#define ACPICA_THREAD_C

#include "acpi.h"

#include <acpica/thread.h>

ACPI_THREAD_ID AcpiOsGetThreadId() {
	return 0;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context) {
	return AE_ERROR;
}

void AcpiOsSleep(UINT64 Milliseconds) {
	return;
}

void AcpiOsStall(UINT32 Microseconds) {
	return;
}

void AcpiOsWaitEventsComplete() {
	return;
}

ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue) {
	return AE_ERROR;
}

ACPI_STATUS AcpiOsWaitCommandReady() {
	return AE_ERROR;
}

ACPI_STATUS AcpiOsNotifyCommandComplete() {
	return AE_ERROR;
}

#endif /* ACPICA_THREAD_C */
