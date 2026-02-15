/* apic_init.h - Local advanced programmable interrupt controller initialization interface */
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

#ifndef DRIVERS_APIC_APIC_INIT_H
#define DRIVERS_APIC_APIC_INIT_H

#include <stdint.h>

extern void apic_init(void);

extern void apic_init_ap(void);

extern void apic_nmi_enab(void);

extern void apic_disab(void);

extern void apic_timer_calib(uint8_t id);

extern uint8_t apic_get_bsp_id(void);

extern void apic_start_ap(void);

#endif /* DRIVERS_APIC_APIC_INIT_H */
