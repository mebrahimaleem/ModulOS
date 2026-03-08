/* driver_database.c - SATA drivers database */
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

#include <sata/driver_database.h>

#include <pcie/generic_database.h>

#ifdef AHCI
#include <ahci/ahci.h>
#endif

#ifdef AHCI

#ifndef U_AHCI
#define U_AHCI
#endif /* U_AHCI */

#endif /* AHCI */

#define SATA_LIMIT 0x02
static generic_driver_t sata_table[SATA_LIMIT + 1] = {
#ifdef U_AHCI
	[0x01] = ahci_generic
#endif /* U_AHCI */
};

void sata_driver_database(
		uint16_t seg,
		uint8_t bus,
		uint8_t dev,
		uint8_t func,
		uint8_t class_code,
		uint8_t subclass,
		uint8_t prog_if,
		uint8_t rev_id) {
	if (prog_if > SATA_LIMIT) {
		return;
	}

	if (sata_table[prog_if]) {
		sata_table[prog_if](seg, bus, dev, func, class_code, subclass, prog_if, rev_id);
	}
}
