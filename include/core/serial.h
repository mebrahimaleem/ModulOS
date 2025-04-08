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
#include <stdarg.h>

#define SERIAL1  1
#define SERIAL2  2
#define SERIAL3  4
#define SERIAL4  8
#define SERIAL5  16
#define SERIAL6  32
#define SERIAL7  64
#define SERIAL8  128

#ifdef DEBUG
#define DEBUG_LOG(fmt) \
	serialPrintf(1, "[DEBUG]\t"); \
	serialPrintf(1, fmt); \
	serialPrintf(1, "\n"); \
	serialPrintf(2, "[DEBUG]\t"); \
	serialPrintf(2, fmt); \
	serialPrintf(2, "\n")
#define DEBUG_LOGF(fmt, ...) \
	serialPrintf(1, "[DEBUG]\t"); \
	serialPrintf(1, fmt, __VA_ARGS__); \
	serialPrintf(1, "\n"); \
	serialPrintf(2, "[DEBUG]\t"); \
	serialPrintf(2, fmt, __VA_ARGS__); \
	serialPrintf(2, "\n")
#else
#define DEBUG_LOG(fmt)
#define DEBUG_LOGF(fmt, ...)
#endif

#define INFO_LOG(fmt) \
	serialPrintf(1, "[INFO]\t"); \
	serialPrintf(1, fmt); \
	serialPrintf(1, "\n"); \
	serialPrintf(2, "[INFO]\t"); \
	serialPrintf(2, fmt); \
	serialPrintf(2, "\n")
#define INFO_LOGF(fmt, ...) \
	serialPrintf(1, "[INFO]\t"); \
	serialPrintf(1, fmt, __VA_ARGS__); \
	serialPrintf(1, "\n"); \
	serialPrintf(2, "[INFO]\t"); \
	serialPrintf(2, fmt, __VA_ARGS__); \
	serialPrintf(2, "\n")

#define PANIC_LOG(fmt) \
	serialPrintf(1, "[PANIC]\t"); \
	serialPrintf(1, fmt); \
	serialPrintf(1, "\n"); \
	serialPrintf(2, "[PANIC]\t"); \
	serialPrintf(2, fmt); \
	serialPrintf(2, "\n")
#define PANIC_LOGF(fmt, ...) \
	serialPrintf(1, "[PANIC]\t"); \
	serialPrintf(1, fmt, __VA_ARGS__); \
	serialPrintf(1, "\n"); \
	serialPrintf(2, "[PANIC]\t"); \
	serialPrintf(2, fmt, __VA_ARGS__); \
	serialPrintf(2, "\n")

void serialinit(void);

void serialWriteStr(uint8_t com, const char* str);

void serialPrintf(uint8_t com, const char* fmt, ...);
void serialVprintf(uint8_t com, const char* fmt, va_list va);

#endif /* CORE_SERIAL_H */

