/* osl.c - ACPICA Modulos OSL */
/* Copyright (C) 2025  Ebrahim Aleem
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY without even the implied warranty of {
* }
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>
*/

#include <stdint.h>
#include <stdarg.h>

#include <acpica_osl/osl.h>

#include <acpica_osl/acpica_include.h>

#include <serial/serial_print.h>
#include <pci/pci_config.h>

#include <kernel/core/logging.h>
#include <kernel/core/alloc.h>
#include <kernel/core/paging.h>
#include <kernel/core/panic.h>
#include <kernel/core/ports.h>
#include <kernel/core/mm.h>
#include <kernel/core/kentry.h>

ACPI_STATUS AcpiOsInitialize(void) {
	return AE_OK;
}

ACPI_THREAD_ID AcpiOsGetThreadId(void) {
	//TODO: implement
	//logging_log_warning("Call to unfinished AcpiOsGetThreadId");
	return 1;
}

void ACPI_INTERNAL_XFACE AcpiOsPrintf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	serial_log(SEVERITY_NON, fmt, args);
	va_end(args);
}

void ACPI_INTERNAL_XFACE AcpiOsVprintf(const char* fmt, va_list args) {
	serial_log(SEVERITY_NON, fmt, args);
}

void* AcpiOsAllocate(ACPI_SIZE size) {
	return kmalloc(size);
}

void AcpiOsFree(void* ptr) {
	//TODO: implement
	//logging_log_warning("Call to unfinished AcpiOsFree 0x%x64", ptr);
	(void)ptr;
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 cap, UINT32 init, ACPI_SEMAPHORE* handle) {
	//TODO: implement
	//logging_log_warning("Call to unfinished AcpiOsCreateSemaphore");
	(void)cap;
	(void)init;
	*handle = (void*)1;
	return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE handle) {
	//TODO: implement
	//logging_log_warning("Call to unfinished AcpiOsDeleteSemaphore");
	(void)handle;
	return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE handle, UINT32 units, UINT16 timeout) {
	//TODO: implement
	//logging_log_warning("Call to unfinished AcpiOsWaitSemaphore");
	(void)handle;
	(void)units;
	(void)timeout;
	return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE handle, UINT32 units) {
	//TODO: implement
	//logging_log_warning("Call to unfinished AcpiOsSignalSemaphore");
	(void)handle;
	(void)units;
	return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK* handle) {
	//TODO: implement
	*handle = (void*)1;
	//logging_log_warning("Call to unfinished AcpiOsCreateLock");
	return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK handle) {
	//TODO: implement
	//logging_log_warning("Call to unfinished AcpiOsCreateLock");
	(void)handle;
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK handle) {
	//TODO: implement
	//logging_log_warning("Call to unfinished AcpiOsAcquireLock");
	(void)handle;
	return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK handle, ACPI_CPU_FLAGS flg) {
	//TODO: implement
	//logging_log_warning("Call to unfinished AcpiOsReleaseLock");
	(void)handle;
	(void)flg;
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS paddr, UINT64* val, UINT32 width) {
	//TODO: implement
	logging_log_warning("Call to unfinished AcpiOsReadMemory");
	(void)paddr;
	(void)val;
	(void)width;
	return AE_SUPPORT;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS paddr, UINT64 val, UINT32 width) {
	//TODO: implement
	logging_log_warning("Call to unfinished AcpiOsWriteMemory");
	(void)paddr;
	(void)val;
	(void)width;
	return AE_SUPPORT;
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS port, UINT32* val, UINT32 width) {
	if ((uint16_t)port != port) {
		logging_log_error("Truncating port to uint16_t changes value: 0x%x64", port);
		return AE_SUPPORT;
	}

	switch (width) {
		case 8:
			*val = inb((uint16_t)port);
			return AE_OK;
		case 16:
			*val = inw((uint16_t)port);
			return AE_OK;
		case 32:
			*val = ind((uint16_t)port);
			return AE_OK;
		default:
			logging_log_error("Cannot read port @ 0x%x64 with width %d64", (uint64_t)port, (uint64_t)width);
			return AE_SUPPORT;
	}
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS port, UINT32 val, UINT32 width) {
	if ((uint16_t)port != port) {
		logging_log_error("Truncating port to uint16_t changes value: 0x%x64", port);
		return AE_SUPPORT;
	}

	switch (width) {
		case 8:
			outb((uint16_t)port, (uint8_t)val);
			return AE_OK;
		case 16:
			outw((uint16_t)port, (uint16_t)val);
			return AE_OK;
		case 32:
			outd((uint16_t)port, (uint32_t)val);
			return AE_OK;
		default:
			logging_log_error("Cannot write port @ 0x%x64 with width %d64", (uint64_t)port, (uint64_t)width);
			return AE_SUPPORT;
	}
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 level, ACPI_OSD_HANDLER handler, void* cntx) {
	//TODO: implement
	logging_log_warning("Call to unfinished AcpiOsInstallInterruptHandler");
	(void)level;
	(void)handler;
	(void)cntx;
	return AE_SUPPORT;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 level, ACPI_OSD_HANDLER handler) {
	//TODO: implement
	logging_log_warning("Call to unfinished AcpiOsRemoveInterruptHandler");
	(void)level;
	(void)handler;
	return AE_SUPPORT;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE type, ACPI_OSD_EXEC_CALLBACK callback, void* cntx) {
	//TODO: implement
	logging_log_warning("Call to unfinished AcpiOsExecute");
	(void)type;
	(void)callback;
	(void)cntx;
	return AE_SUPPORT;
}

void AcpiOsSleep(UINT64 ms) {
	//TODO: implement
	logging_log_warning("Call to unfinished AcpiOsSleep");
	(void)ms;
}

void AcpiOsStall(UINT32 us) {
	//TODO: implement
	logging_log_warning("Call to unfinished AcpiOsStall");
	(void)us;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES* predefined, ACPI_STRING* new_value) {
	(void)predefined;
	*new_value = NULL;
	return AE_OK;
}

void AcpiOsWaitEventsComplete(void) {
	//TODO: implement
	logging_log_warning("Call to unfinished AcpiOsWaitEventsComplete");
}

void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS paddr, ACPI_SIZE len) {
	const uint64_t page_base = paddr & PAGE_BASE_MASK;
	const uint64_t adj = paddr - page_base;
	len += adj;
	uint64_t order = mm_lowest_order(len);
	if (order == (uint64_t)-1) {
		logging_log_error("Could not allocate contigious virtual block of 0x%x64", (uint64_t)len);
		panic(PANIC_NO_MEM);
	}
	const uint64_t vaddr = mm_alloc_dv((enum mm_order_t)order);
	if (!vaddr) {
		logging_log_error("Could not allocate contigious virtual block of 0x%x64", (uint64_t)len);
		panic(PANIC_NO_MEM);
	}

	for (uint64_t i = 0; i < len; i += PAGE_SIZE_4K) {
		paging_map(vaddr + i, page_base + i, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
	}

	return (void*)(vaddr + adj);
}

void AcpiOsUnmapMemory(void* vaddr, ACPI_SIZE len) {
	logging_log_warning("Call to unfinished AcpiOsUnmapMemory");
	(void)vaddr;
	(void)len;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER* existing, ACPI_TABLE_HEADER** override) {
	(void)existing;
	*override = NULL;
	return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(
		ACPI_TABLE_HEADER* existing,
		ACPI_PHYSICAL_ADDRESS* addr,
		UINT32* len) {
	(void)existing;
	*addr = 0;
	(void)len;
	return AE_OK;
}

UINT64 AcpiOsGetTimer(void) {
	logging_log_warning("Call to unfinished AcpiOsGetTimer");
	return 0;
}

ACPI_STATUS AcpiOsSignal(UINT32 func, void* info) {
	logging_log_warning("Call to unfinished AcpiOsSignal");
	(void)func;
	(void)info;
	return AE_OK;
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID* id, UINT32 reg, UINT64* val, UINT32 width) {
	*val = pci_read_conf_noalign(
			(uint8_t)reg,
			(uint8_t)id->Function,
			(uint8_t)id->Device,
			(uint8_t)id->Bus,
			(uint8_t)width);
	return AE_OK;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID* id, UINT32 reg, UINT64 val, UINT32 width) {
	pci_write_conf_noalign(
			(uint8_t)reg,
			(uint8_t)id->Function,
			(uint8_t)id->Device,
			(uint8_t)id->Bus,
			(uint8_t)width,
			val);
	return AE_OK;
}

ACPI_STATUS AcpiOsTerminate(void) {
	return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void) {
	return (uint64_t)&boot_context.rsdp - KERNEL_VMA;
}
