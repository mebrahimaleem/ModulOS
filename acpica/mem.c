/* mem.c - memory management for ACPICA */
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

#ifndef ACPICA_MEM_C
#define ACPICA_MEM_C

#include "acpi.h"

#include <core/memory.h>

#include <acpica/mem.h>

/*
ACPI_STATUS AcpiOsCreateCache(char *CacheName, UINT16 ObjectSize, UINT16 MaxDepth, ACPI_CACHE_T **ReturnCache) {
	return AE_ERROR;
}
*/

/*
ACPI_STATUS AcpiOsDeleteCache(ACPI_CACHE_T *Cache) {
	return AE_ERROR;
}
*/

/*
ACPI_STATUS AcpiOsPurgeCache(ACPI_CACHE_T *Cache) {
	return AE_ERROR;
}
*/

/*
void* AcpiOsAcquireObject(ACPI_CACHE_T *Cache) {
	return NULL;
}
*/

/*
ACPI_STATUS AcpiOsReleaseObject(ACPI_CACHE_T *Cache, void *Object) {
	return AE_ERROR;
}
*/

void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
	return NULL;
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Length) {
	return;
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress) {
	return AE_ERROR;
}

void* AcpiOsAllocate(ACPI_SIZE Size) {
	return kmalloc(kheap_private, Size);
}

void AcpiOsFree(void *Memory) {
	return;
}

BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length) {
	return 0;
}

BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length) {
	return 0;
}

#endif /* ACPICA_MEM_C */
