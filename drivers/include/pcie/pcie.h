/* pcie_init.h - Peripheral Controller Interface Express driver interface */
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

#ifndef DRIVERS_INCLUDE_PCIE_H
#define DRIVERS_INCLUDE_PCIE_H

#include <stdint.h>

#define PCI_ID_REG		0x00
#define PCI_CMD_REG		0x04
#define PCI_CLASS_REG	0x08
#define PCI_ATTR_REG	0x0C
#define PCI_BAR0_REG	0x10
#define PCI_BAR1_REG	0x14
#define PCI_BAR2_REG	0x18
#define PCI_BAR3_REG	0x1C
#define PCI_BAR4_REG	0x20
#define PCI_BAR5_REG	0x24

#define PCI_CMD_IOSE	0x001
#define PCI_CMD_MSE		0x002
#define PCI_CMD_BME		0x004
#define PCI_CMD_ID		0x400

#define PCI_BAR_BA_MAKS	0xFFFFE000

extern uint32_t pcie_read(uint16_t segment, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off);
extern void pcie_write(uint16_t segment, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off, uint32_t val);

#endif /* DRIVERS_INCLUDE_PCIE_H */
