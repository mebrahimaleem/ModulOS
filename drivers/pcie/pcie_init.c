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

#include <acpi/tables.h>

#include <kernel/core/alloc.h>
#include <kernel/core/mm.h>
#include <kernel/core/paging.h>
#include <kernel/core/logging.h>

#include <kernel/lib/kmemset.h>

#define DEV_BASE(ecam, bus, dev) (ecam + (((uint64_t)bus << 20) | ((uint64_t)dev << 15)))

#define DEVICE_EXISTS(seg, bus, dev)	(VENDOR_ID(seg, bus, dev) != 0xFFFF)

#define VENDOR_ID(seg, bus, dev) 		(0xFFFF & pcie_read(seg, bus, dev, 0, 0x0))
#define DEVICE_ID(seg, bus, dev) 		(0xFFFF & (pcie_read(seg, bus, dev, 0, 0x0) >> 16))
#define HEADER_TYPE(seg, bus, dev)	(0x7F & (pcie_read(seg, bus, dev, 0, 0xC) >> 16))

#define PCI_BRIDGE_SEC_NUM(seg, bus, dev)	(0xFF & (pcie_read(seg, bus, dev, 0, 0x18) >> 8))

extern uint64_t*** ecam_dev_bases;

void pcie_init(void) {
	uint64_t mcfg_handle;
	struct acpi_mcfg_conf_t conf;
	uint16_t max_seg = 0;
	uint16_t j;
	uint16_t max_bus;
	uint16_t i;
	uint64_t vaddr = 0;
	uint8_t k;

	acpi_parse_mcfg_conf_start(&mcfg_handle);
	acpi_parse_mcfg_conf(&conf, &mcfg_handle);
	while (mcfg_handle) {	
		if (conf.segment > max_seg) {
			max_seg = conf.segment;
		}

		acpi_parse_mcfg_conf(&conf, &mcfg_handle);
	}

	max_seg++;

	ecam_dev_bases = kmalloc(sizeof(uint64_t*) * max_seg );	

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
			ecam_dev_bases[i] = 0;
			continue;
		}

		ecam_dev_bases[i] = kmalloc(sizeof(uint64_t) * (max_bus + 1));
		kmemset(ecam_dev_bases[i], 0, sizeof(uint64_t) * (max_bus + 1));

		acpi_parse_mcfg_conf_start(&mcfg_handle);
		acpi_parse_mcfg_conf(&conf, &mcfg_handle);
		while (mcfg_handle) {
			if (conf.segment == i) {
				for (j = conf.bus_start; j <= conf.bus_end; j++) {
					ecam_dev_bases[i][j] = kmalloc(32 * sizeof(uint64_t));
					for (k = 0; k < 32; k++) {
						vaddr = mm_alloc_v(PAGE_SIZE_4K);
						paging_map(vaddr, DEV_BASE(conf.base, j, k), PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
						ecam_dev_bases[i][j][k] = vaddr;
					}
				}
			}

			acpi_parse_mcfg_conf(&conf, &mcfg_handle);
		}
	}
}

static void enumerate_bus(uint16_t seg, uint8_t bus) {
	uint8_t dev;
	logging_log_debug("Enumerating pcie bus %u.%u", seg, bus);
	for (dev = 0; dev < 32; dev++) {
		if (!DEVICE_EXISTS(seg, bus, dev)) {
			continue;
		}
		switch (HEADER_TYPE(seg, bus, dev)) {
			case 0:
				logging_log_debug("Found device 0x%x:0x%x on %u.%u.%u",
						VENDOR_ID(seg, bus, dev), DEVICE_ID(seg, bus, dev), seg, bus, dev);
				break;
			case 1:
				logging_log_debug("Found PCI bridge on %u.%u.%u", seg, bus, dev);
				enumerate_bus(seg, PCI_BRIDGE_SEC_NUM(seg, bus, dev));
				break;
			case 2:
				logging_log_debug("Found CardBus bridge on %u.%u.%u", seg, bus, dev);
				break;
			default:
				logging_log_error("Bad device on pcie %u.%u.%u", seg, bus, dev);
				break;
		}
	}
	logging_log_debug("Done enumerating pcie bus %u.%u", seg, bus);
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
