/* portio.h - port io functions */
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

#ifndef CORE_PORTIO_H
#define CORE_PORTIO_H

#include <stdint.h>

void outb(uint16_t addr, uint8_t x);
void outw(uint16_t addr, uint16_t x);
void outd(uint16_t addr, uint32_t x);

uint8_t inb(uint16_t addr);
uint16_t inw(uint16_t addr);
uint32_t ind(uint16_t addr);

void iowait(void);

#endif /* CORE_PORTIO_H */

