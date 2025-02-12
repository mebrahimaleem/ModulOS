/* serial.h - serial port functions */
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

#ifndef CORE_SERIAL_H
#define CORE_SERIAL_H

#include <stdint.h>

#define SERIAL1  1
#define SERIAL2  2
#define SERIAL3  4
#define SERIAL4  8
#define SERIAL5  16
#define SERIAL6  32
#define SERIAL7  64
#define SERIAL8  128

void serialinit(void);

void serialWriteStr(uint8_t com, const char* str);

#endif /* CORE_SERIAL_H */

