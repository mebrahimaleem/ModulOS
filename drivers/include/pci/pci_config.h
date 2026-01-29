/* pci_config.h - Peripheral Controller Interface configuration space access interface */
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

#ifndef DRIVERS_PIC_PIC_CONFIG
#define DRIVERS_PIC_PIC_CONFIG

#include <stdint.h>

extern uint32_t pci_read_conf(uint8_t reg, uint8_t fun, uint8_t dev, uint8_t bus);
extern void pci_write_conf(uint8_t reg, uint8_t fun, uint8_t dev, uint8_t bus, uint32_t data);

extern uint64_t pci_read_conf_noalign(uint8_t reg, uint8_t fun, uint8_t dev, uint8_t bus, uint8_t width);
extern void pci_write_conf_noalign(uint8_t reg, uint8_t fun, uint8_t dev, uint8_t bus, uint8_t width, uint64_t data);

#endif /* DRIVERS_PIC_PIC_CONFIG */
