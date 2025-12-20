/* serial.h - serial driver interface */
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

#ifndef DRIVERS_SERIAL_SERIAL
#define DRIVERS_SERIAL_SERIAL

#include <stdint.h>

extern void serial_init_com1(void);
extern void serial_init_com2(void);

extern void serial_write_com1(uint8_t b);
extern void serial_write_com2(uint8_t b);

#endif /* DRIVERS_SERIAL_SERIAL */
