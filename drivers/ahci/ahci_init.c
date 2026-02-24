/* ahci_init.c - Advanced Host Controller Interface initialization */
/* Copyright (C) 2026  Ebrahim Aleem
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

#include <stdint.h>

#include <ahci/ahci_init.h>

#include <kernel/core/logging.h>
#include <kernel/core/scheduler.h>
#include <kernel/core/process.h>
#include <kernel/core/alloc.h>

struct ahci_t {
	uint16_t seg;
	uint8_t bus;
	uint8_t dev;
	uint8_t func;
	uint8_t class_code;
	uint8_t subclass;
	uint8_t prog_if;
	uint8_t rev_id;
};

static void ahci_init(void* cntx) {
	struct ahci_t* ahci = cntx;

	logging_log_info("AHCI driver initialization for %u/%u/%u/%u at %u.%u.%u.%u.",
			ahci->class_code, ahci->subclass, ahci->prog_if, ahci->rev_id, ahci->seg, ahci->bus, ahci->dev, ahci->func);
}

void ahci_generic(
		uint16_t seg,
		uint8_t bus,
		uint8_t dev,
		uint8_t func,
		uint8_t class_code,
		uint8_t subclass,
		uint8_t prog_if,
		uint8_t rev_id) {

	struct ahci_t* ahci = kmalloc(sizeof(struct ahci_t));
	ahci->seg = seg;
	ahci->bus = bus;
	ahci->dev = dev;
	ahci->func = func;
	ahci->class_code = class_code;
	ahci->subclass = subclass;
	ahci->prog_if = prog_if;
	ahci->rev_id = rev_id;

	scheduler_schedule(process_from_func(ahci_init, ahci));
}
