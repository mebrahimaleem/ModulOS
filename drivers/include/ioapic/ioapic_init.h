/* ioapic_init.h - IO/APIC initialization interface */
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

#ifndef DRIVERS_IOAPIC_IOAPIC_INIT_H
#define DRIVERS_IOAPIC_IOAPIC_INIT_H

#include <stdint.h>

#define IOAPIC_REDIR_TRG_EDG	0x00000
#define IOAPIC_REDIR_TRG_LVL	0x08000
#define IOAPIC_REDIR_POL_HI		0x00000
#define IOAPIC_REDIR_POL_LO		0x02000
#define IOAPIC_REDIR_MASK			0x10000

#define IOAPIC_REDIR_NMI	0x500

extern void ioapic_init(void);

extern void ioapic_conf_gsi(uint64_t gsi, uint8_t v, uint32_t flg, uint8_t dest);

#endif /* DRIVERS_IOAPIC_IOAPIC_INIT_H */
