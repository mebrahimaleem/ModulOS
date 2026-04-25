/* interrupts.h - serial interrupts interface */
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

#ifndef DRIVERS_SERIAL_INTERRUPTS_H
#define DRIVERS_SERIAL_INTERRUPTS_H

#include <stdint.h>

extern void serial_init_interrupts(void);

extern void serial_isr_dispatch(uint16_t com);

#endif /* DRIVERS_SERIAL_INTERRUPTS_H */
