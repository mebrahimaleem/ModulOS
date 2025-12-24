/* routing.c - IO/APIC interrupt routing */
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

#include <stdint.h>

#include <ioapic/routing.h>

#include <kernel/core/alloc.h>

#define NUM_LEGACY	16

struct gsi_route_t {
	const char* purpose;
	enum {
		GSI_TYPE_LEGACY
	} type;
};

static struct gsi_route_t* gsi_routing;

void ioapic_routing_init(uint64_t num_gsi) {
	gsi_routing = kmalloc(num_gsi * sizeof(struct gsi_route_t));
}

