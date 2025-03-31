/* mutexsync.c - mutual exclusion and synchronization for ACPICA */
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

#ifndef ACPICA_MUTEXSYNC_C
#define ACPICA_MUTEXSYNC_C

#include <stdint.h>

#include <core/atomic.h>

#include "acpi.h"

#include <acpica/mutexsync.h>

uint8_t acpi_acquire_global_lock(void* facsPtr) {
	//TODO: implement
	return 0;
}

uint8_t acpi_release_global_lock(void* facsPtr) {
	//TODO: implement
	return 0;
}

ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle) {
	*OutHandle = kcreateMutex();
	return AE_OK;
}

void AcpiOsDeleteMutex(ACPI_MUTEX Handle) {
	kdeleteMutex(Handle);
	return;
}

ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout) {
	//TODO: implement timeout
	kacquireMutex(Handle);
	return AE_OK;
}

void AcpiOsReleaseMutex(ACPI_MUTEX Handle) {
	kreleaseMutex(Handle);
	return;
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle) {
	*OutHandle = kcreateSemaphore(InitialUnits, MaxUnits);
	return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
	kdeleteSemaphore(Handle);
	return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout) {
	//TODO: implement timeout
	kacquireSemaphore(Handle, Units);
	return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
	kreleaseSemaphore(Handle, Units);
	return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
	*OutHandle = kcreateSpinlock();
	return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle) {
	kdeleteSpinlock(Handle);
	return;
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
	kacquireSpinlock(Handle);
	return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) {
	kreleaseSpinlock(Handle);
	return;
}

#endif /* ACPICA_MUTEXSYNC_C */
