/* pcie_init.c - Peripheral Controller Interface Express initialization */
/* Copyright (C) 2025-2026  Ebrahim Aleem
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

#include <pcie/pcie_init.h>
#include <pcie/pcie.h>
#include <pcie/generic_database.h>

#include <acpi/tables.h>

#include <kernel/core/alloc.h>
#include <kernel/core/mm.h>
#include <kernel/core/paging.h>
#include <kernel/core/logging.h>

#include <kernel/lib/kmemset.h>

#define FUN_BASE(ecam, bus, dev, fun) (ecam + (((uint64_t)bus << 20) | ((uint64_t)dev << 15) | ((uint64_t)fun << 12)))

#define FUNCTION_EXISTS(seg, bus, dev, func)	(VENDOR_ID(seg, bus, dev, func) != 0xFFFF)

#define VENDOR_ID(seg, bus, dev, func) 		(0xFFFF & pcie_read(seg, bus, dev, func, 0x0))
#define DEVICE_ID(seg, bus, dev, func) 		(0xFFFF & (pcie_read(seg, bus, dev, func, 0x0) >> 16))
#define REV_ID(seg, bus, dev, func)				(0xFF & pcie_read(seg, bus, dev, func, 0x8))
#define PROG_IF(seg, bus, dev, func)			(0xFF & (pcie_read(seg, bus, dev, func, 0x8) >> 8))
#define SUBCLASS(seg, bus, dev, func)			(0xFF & (pcie_read(seg, bus, dev, func, 0x8) >> 16))
#define CLASS_CODE(seg, bus, dev, func)		(0xFF & (pcie_read(seg, bus, dev, func, 0x8) >> 24))
#define HEADER_TYPE(seg, bus, dev, func)	(0x7F & (pcie_read(seg, bus, dev, func, 0xC) >> 16))
#define IS_MULTIFUNC(seg, bus, dev)				(0x80 & (pcie_read(seg, bus, dev, 0, 0xC) >> 16))

#define IS_MULTIFUNC_DIRECT(base)		(0x80 & (*(volatile uint32_t*)(base + 0x0C) >> 16))

#define PCI_BRIDGE_SEC_NUM(seg, bus, dev, func)	(0xFF & (pcie_read(seg, bus, dev, func, 0x18) >> 8))

#define DISABLE_INT(seg, bus, dev, func)	pcie_write(seg, bus, dev, func, 0x4, pcie_read(seg, bus, dev, 0, 0x4) | 0x400);

extern uint64_t**** ecam;

static void enumerate_bus(uint16_t seg, uint8_t bus);

static void configure_function(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func) {
	DISABLE_INT(seg, bus, dev, func);

	const uint16_t vendor_id = VENDOR_ID(seg, bus, dev, func);
	const uint16_t device_id = DEVICE_ID(seg, bus, dev, func);
	const uint8_t class_code = CLASS_CODE(seg, bus, dev, func);
	const uint8_t subclass = SUBCLASS(seg, bus, dev, func);
	const uint8_t prog_if = PROG_IF(seg, bus, dev, func);
	const uint8_t rev_id = REV_ID(seg, bus, dev, func);


	switch (HEADER_TYPE(seg, bus, dev, func)) {
		case 0:
			logging_log_debug("Found device function 0x%x:0x%x (%u/%u/%u/%u) on %u.%u.%u.%u",
					vendor_id, device_id, class_code, subclass, prog_if, rev_id, seg, bus, dev, func);
			break;
		case 1:
			logging_log_debug("Found PCI bridge function 0x%x:0x%x (%u/%u/%u/%u) on %u.%u.%u.%u",
					vendor_id, device_id, class_code, subclass, prog_if, rev_id, seg, bus, dev, func);
			enumerate_bus(seg, PCI_BRIDGE_SEC_NUM(seg, bus, dev, func));
			break;
		case 2:
			logging_log_debug("Found CardBus bridge function 0x%x:0x%x (%u/%u/%u/%u) on %u.%u.%u.%u",
					vendor_id, device_id, class_code, subclass, prog_if, rev_id, seg, bus, dev, func);
			break;
		default:
			logging_log_debug("Found unkown function 0x%x:0x%x (%u/%u/%u/%u) on %u.%u.%u.%u",
					vendor_id, device_id, class_code, subclass, prog_if, rev_id, seg, bus, dev, func);
			break;
	}

	pcie_generic_init(seg, bus, dev, func, class_code, subclass, prog_if, rev_id);
}

static void enumerate_bus(uint16_t seg, uint8_t bus) {
	uint8_t dev;
	uint8_t func;
	logging_log_debug("Enumerating pcie bus %u.%u", seg, bus);
	for (dev = 0; dev < 32; dev++) {
		if (!FUNCTION_EXISTS(seg, bus, dev, 0)) {
			continue;
		}

		configure_function(seg, bus, dev, 0);

		if (IS_MULTIFUNC(seg, bus, dev)) {
			for (func = 1; func < 8; func ++) {
				if (FUNCTION_EXISTS(seg, bus, dev, func)) {
					configure_function(seg, bus, dev, func);
				}
			}
		}
	}
	logging_log_debug("Done enumerating pcie bus %u.%u", seg, bus);
}

void pcie_init(void) {
	uint64_t mcfg_handle;
	struct acpi_mcfg_conf_t conf;
	uint16_t max_seg = 0;
	uint16_t j;
	uint16_t max_bus;
	uint16_t i;
	uint8_t k, l;
	uint64_t vaddr;

	acpi_parse_mcfg_conf_start(&mcfg_handle);
	acpi_parse_mcfg_conf(&conf, &mcfg_handle);
	while (mcfg_handle) {	
		if (conf.segment > max_seg) {
			max_seg = conf.segment;
		}

		acpi_parse_mcfg_conf(&conf, &mcfg_handle);
	}

	max_seg++;

	ecam = kmalloc(sizeof(uint64_t***) * max_seg );	

	for (i = 0; i < max_seg; i++) {
		max_bus = 0;
		j = 0;

		acpi_parse_mcfg_conf_start(&mcfg_handle);
		acpi_parse_mcfg_conf(&conf, &mcfg_handle);
		while (mcfg_handle) {
			if (conf.segment == i && conf.bus_end >= max_bus) {
				max_bus = conf.bus_end;
				j = 1;
			}

			acpi_parse_mcfg_conf(&conf, &mcfg_handle);
		}
		
		if (!j) {
			ecam[i] = 0;
			continue;
		}

		ecam[i] = kmalloc(sizeof(uint64_t**) * (max_bus + 1));
		kmemset(ecam[i], 0, sizeof(uint64_t) * (max_bus + 1));

		acpi_parse_mcfg_conf_start(&mcfg_handle);
		acpi_parse_mcfg_conf(&conf, &mcfg_handle);
		while (mcfg_handle) {
			if (conf.segment == i) {
				for (j = conf.bus_start; j <= conf.bus_end; j++) {
					ecam[i][j] = kmalloc(32 * sizeof(uint64_t*));
					for (k = 0; k < 32; k++) {
						vaddr = mm_alloc_v(PAGE_SIZE_4K);
						paging_map(vaddr, FUN_BASE(conf.base, j, k, 0), PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
						
						if (IS_MULTIFUNC_DIRECT(vaddr)) {
							ecam[i][j][k] = kmalloc(8 * sizeof(uint64_t));
							ecam[i][j][k][0] = vaddr;
							for (l = 1; l < 8; l++) {
								vaddr = mm_alloc_v(PAGE_SIZE_4K);
								paging_map(vaddr, FUN_BASE(conf.base, j, k, l), PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
								ecam[i][j][k][l] = vaddr;
							}
						}
						else {
							ecam[i][j][k] = kmalloc(sizeof(uint64_t));
							ecam[i][j][k][0] = vaddr;
						}

					}
				}
			}

			acpi_parse_mcfg_conf(&conf, &mcfg_handle);
		}
	}
}

void pcie_enumerate(void) {
	uint64_t mcfg_handle;
	struct acpi_mcfg_conf_t conf;

	acpi_parse_mcfg_conf_start(&mcfg_handle);
	acpi_parse_mcfg_conf(&conf, &mcfg_handle);
	while (mcfg_handle) {
		enumerate_bus(conf.segment, conf.bus_start);
		acpi_parse_mcfg_conf(&conf, &mcfg_handle);
	}
}
