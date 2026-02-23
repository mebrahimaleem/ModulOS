/* pcie_init.c - Peripheral Controller Interface Express driver */
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

#include <stdint.h>

#include <pcie/pcie.h>

#define FUN_OFF(dev, fun) (((uint64_t)dev << 15) | ((uint64_t)fun << 12))

uint64_t** ecam_bus_bases;

uint32_t pcie_read(uint16_t segment, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off) {
	return *(volatile uint32_t*)(ecam_bus_bases[segment][bus] | FUN_OFF(dev, fun) + off);
}

void pcie_write(uint16_t segment, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off, uint32_t val) {
	*(volatile uint32_t*)(ecam_bus_bases[segment][bus] | FUN_OFF(dev, fun) + off) = val;
}
