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

#include <acpi/tables.h>

#include <kernel/core/alloc.h>
#include <kernel/core/mm.h>
#include <kernel/core/paging.h>

#include <kernel/lib/kmemset.h>

#define BUS_BASE(ecam, bus) (ecam + ((uint64_t)bus << 20))

extern uint64_t** ecam_bus_bases;

void pcie_init(void) {
	uint64_t mcfg_handle;
	struct acpi_mcfg_conf_t conf;
	uint16_t max_seg = 0;
	uint16_t j;
	uint8_t max_bus;
	uint16_t i;
	uint64_t vaddr = 0;

	acpi_parse_mcfg_conf_start(&mcfg_handle);
	while (mcfg_handle) {
		acpi_parse_mcfg_conf(&conf, &mcfg_handle);
	
		if (conf.segment > max_seg) {
			max_seg = conf.segment;
		}
	}

	ecam_bus_bases = kmalloc(sizeof(uint64_t*) * max_seg);	

	for (i = 0; i < max_seg; i++) {
		max_bus = 0;
		j = 0;

		acpi_parse_mcfg_conf_start(&mcfg_handle);
		while (mcfg_handle) {
			acpi_parse_mcfg_conf(&conf, &mcfg_handle);

			if (conf.segment == i && conf.bus_end >= max_bus) {
				max_bus = conf.bus_end;
				j = 1;
			}
		}
		
		if (!j) {
			ecam_bus_bases[i] = 0;
			continue;
		}

		ecam_bus_bases[i] = kmalloc(sizeof(uint64_t) * (max_bus + 1));
		kmemset(ecam_bus_bases[i], 0, sizeof(uint64_t) * (max_bus + 1));

		acpi_parse_mcfg_conf_start(&mcfg_handle);
		while (mcfg_handle) {
			acpi_parse_mcfg_conf(&conf, &mcfg_handle);

			if (conf.segment == i) {
				for (j = conf.bus_start; j <= conf.bus_end; j++) {
					vaddr = mm_alloc_v(PAGE_SIZE_4K);
					paging_map(vaddr, BUS_BASE(conf.base, j), PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
					ecam_bus_bases[i][j] = vaddr;
				}
			}
		}
	}
}

void pcie_enumerate(void) {
}
