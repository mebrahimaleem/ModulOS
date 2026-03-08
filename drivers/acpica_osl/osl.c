/* osl.c - ACPICA Modulos OSL */
/* Copyright (C) 2025-2026  Ebrahim Aleem
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
#include <pcie/pcie.h>

#include <kernel/core/logging.h>
#include <kernel/core/alloc.h>
#include <kernel/core/paging.h>
#include <kernel/core/panic.h>
#include <kernel/core/ports.h>
#include <kernel/core/mm.h>
#include <kernel/core/kentry.h>
#include <kernel/core/time.h>
#include <kernel/core/clock_src.h>
#include <kernel/core/lock.h>
#include <kernel/core/process.h>

ACPI_STATUS AcpiOsInitialize(void) {
	return AE_OK;
}

ACPI_THREAD_ID AcpiOsGetThreadId(void) {
	return process_get_pid();
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
	kfree(ptr);
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
	*handle = kmalloc(sizeof(uint8_t));
	if (*handle) {
		lock_init(*handle);
		return AE_OK;
	}
	return AE_ERROR;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK handle) {
	kfree(handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK handle) {
	lock_acquire(handle);
	return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK handle, ACPI_CPU_FLAGS flg) {
	(void)flg;
	lock_release(handle);
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
		logging_log_error("Truncating port to uint16_t changes value: 0x%lx", port);
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
			logging_log_error("Cannot read port @ 0x%lx with width %lu", (uint64_t)port, (uint64_t)width);
			return AE_SUPPORT;
	}
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS port, UINT32 val, UINT32 width) {
	if ((uint16_t)port != port) {
		logging_log_error("Truncating port to uint16_t changes value: 0x%lx", port);
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
			logging_log_error("Cannot write port @ 0x%lx with width %lu", (uint64_t)port, (uint64_t)width);
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
	time_busy_wait(ms * TIME_CONV_MS_TO_NS);
}

void AcpiOsStall(UINT32 us) {
	time_busy_wait(us * TIME_CONV_US_TO_NS);
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
	const uint64_t vaddr = mm_alloc_v(len);
	if (!vaddr) {
		logging_log_error("Could not allocate contigious virtual block of 0x%lx", (uint64_t)len);
		return 0;
	}

	for (uint64_t i = 0; i < len; i += PAGE_SIZE_4K) {
		if (!paging_map(vaddr + i, page_base + i, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K)) {
			logging_log_error("Failed to map memory for ACPICA");
			return 0;
		}
	}

	return (void*)(vaddr + adj);
}

void AcpiOsUnmapMemory(void* vaddr, ACPI_SIZE len) {
	const uint64_t page_base = (uint64_t)vaddr & PAGE_BASE_MASK;
	const uint64_t adj = (uint64_t)vaddr - page_base;
	len += adj;
	for (uint64_t i = 0; i < len; i += PAGE_SIZE_4K) {
		paging_unmap(page_base + i, PAGE_4K);
	}
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
	return time_since_init_ns();
}

ACPI_STATUS AcpiOsSignal(UINT32 func, void* info) {
	logging_log_warning("Call to unfinished AcpiOsSignal");
	(void)func;
	(void)info;
	return AE_OK;
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID* id, UINT32 reg, UINT64* val, UINT32 width) {
	uint32_t reg_cur;
	uint32_t reg_end;
	uint32_t read;
	uint64_t constr = 0;
	uint8_t shft = 0;
	switch (width) {
		case 8:
			reg_end = reg + 1;
			break;
		case 16:
			reg_end = reg + 2;
			break;
		case 32:
			reg_end = reg + 4;
			break;
		case 64:
			reg_end = reg + 8;
			break;
		default:
			return AE_SUPPORT;
	}

	for (reg_cur = reg - (reg % 4); reg_cur < reg_end; reg_cur += 4) {
		read = pcie_read(id->Segment, (uint8_t)id->Bus, (uint8_t)id->Device, (uint8_t)id->Function, (uint16_t)reg_cur);
		if (reg_cur < reg) {
			shft = (uint8_t)(8 * (reg - reg_cur));
			constr |= read << shft;
			shft = 32 - shft;
		}
		else {
			constr |= (uint64_t)read >> shft;
			shft += 32;
		}
	}

	switch (width) {
		case 8:
			*val = constr & 0xFF;
			break;
		case 16:
			*val = constr & 0xFFFF;
			break;
		case 32:
			*val = constr & 0xFFFFFFFF;
			break;
		case 64:
			*val = constr;
			break;
	}

	return AE_OK;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID* id, UINT32 reg, UINT64 val, UINT32 width) {
	uint32_t reg_cur;
	uint32_t reg_end;
	uint32_t write;
	uint8_t shft = 0;
	switch (width) {
		case 8:
			reg_end = reg + 1;
			break;
		case 16:
			reg_end = reg + 2;
			break;
		case 32:
			reg_end = reg + 4;
			break;
		case 64:
			reg_end = reg + 8;
			break;
		default:
			return AE_SUPPORT;
	}

	for (reg_cur = reg - (reg % 4); reg_cur < reg_end; reg_cur += 4) {
		if (reg_cur < reg) {
			shft = (uint8_t)(8 * (reg - reg_cur));
			write = (uint32_t)(val << shft);
			shft = 32 - shft;
		}
		else {
			write = (uint32_t)(val >> shft);
			val += 32;
		}

		pcie_write(id->Segment, (uint8_t)id->Bus, (uint8_t)id->Device, (uint8_t)id->Function, (uint16_t)reg_cur, write);
	}

	return AE_OK;
}

ACPI_STATUS AcpiOsTerminate(void) {
	return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void) {
	return (uint64_t)&boot_context.rsdp - KERNEL_VMA;
}
