/* routing.h - IO/APIC interrupt routing interface */
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

#ifndef DRIVERS_IOAPIC_ROUTING_H
#define DRIVERS_IOAPIC_ROUTING_H

#include <stdint.h>

extern void ioapic_routing_init(uint64_t num_gsi);

extern uint64_t ioapic_routing_legacy_gsi(uint8_t isa_irq);

#endif /* DRIVERS_IOAPIC_ROUTING_H */
