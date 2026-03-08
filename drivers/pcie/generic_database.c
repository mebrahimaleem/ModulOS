/* generic_database.c - Peripheral Controller Interface Express generic drivers database */
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

#include <pcie/generic_database.h>

#ifdef AHCI
#include <ahci/ahci.h>
#endif /* AHCI */

#ifdef SATA
#include <sata/driver_database.h>
#endif /* SATA */

#define CLASS_LIMIT	0x13

#ifdef SATA

#ifndef U_MASS_STORAGE_CONTROLLER
#define U_MASS_STORAGE_CONTROLLER
#endif /* U_MASS_STORAGE_CONTROLLER */

#ifndef U_SATA
#define U_SATA
#endif /* U_SATA */

#endif /* SATA */

#ifdef U_MASS_STORAGE_CONTROLLER
#define MASS_STORAGE_CONTROLLER_LIMIT	0x06
static generic_driver_t mass_storage_controller_table[MASS_STORAGE_CONTROLLER_LIMIT + 1] = {
#ifdef U_SATA
	[0x06] = sata_driver_database
#endif /* U_SATA  */
};

static void u_mass_storage_controller(
		uint16_t seg,
		uint8_t bus,
		uint8_t dev,
		uint8_t func,
		uint8_t class_code,
		uint8_t subclass,
		uint8_t prog_if,
		uint8_t rev_id) {
	if (subclass > MASS_STORAGE_CONTROLLER_LIMIT) {
		return;
	}

	if (mass_storage_controller_table[subclass]) {
		mass_storage_controller_table[subclass](seg, bus, dev, func, class_code, subclass, prog_if, rev_id);
	}
}
#endif /* U_MASS_STORAGE_CONTROLLER */

static generic_driver_t class_table[CLASS_LIMIT + 1] = {
#ifdef U_MASS_STORAGE_CONTROLLER
	[0x01] = u_mass_storage_controller
#endif /* U_MASS_STORAGE_CONTROLLER */
};

void pcie_generic_init(
		uint16_t seg,
		uint8_t bus,
		uint8_t dev,
		uint8_t func,
		uint8_t class_code,
		uint8_t subclass,
		uint8_t prog_if,
		uint8_t rev_id) {
	if (class_code > CLASS_LIMIT) {
		return;
	}

	if (class_table[class_code]) {
		class_table[class_code](seg, bus, dev, func, class_code, subclass, prog_if, rev_id);
	}
}
