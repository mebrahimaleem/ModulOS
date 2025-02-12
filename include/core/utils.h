/* utils.h - kernel utils */
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

#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <stdint.h>

void kcopy(void* src, void* dst, uint64_t len);

void kfill(void* dst, uint64_t len, uint8_t val);

/*
 * Converts num to base string and stores it into str of length strlen (excluding null terminating byte)
 */
uint64_t intToString(uint64_t num, uint8_t base, char* str, uint64_t strlen);

void kstrcpy(const char* from, char* to);

#endif /* CORE_UTILS_H */

