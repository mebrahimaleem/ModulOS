/* kmemcpy.c - library memcpy implementation */
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

#include <stddef.h>
#include <stdint.h>

#include <lib/kmemcpy.h>

void* memcpy(void* dest, const void* src, size_t c) __attribute__((weak));

void* memcpy(void* dest, const void* src, size_t c) {
	const uint64_t* src64 = (const uint64_t*)src;
	uint64_t* dest64 = (uint64_t*)dest;

	while (c >= sizeof(uint64_t)) {
		*dest64 = *src64;
		dest64++;
		src64++;
		c -= sizeof(uint64_t);
	}

	const uint8_t* src8 = (const uint8_t*)src64;
	uint8_t* dest8 = (uint8_t*)dest64;

	while (c >= sizeof(uint8_t)) {
		*dest8 = *src8;
		dest8++;
		src8++;
		c -= sizeof(uint8_t);
	}

	return dest;
}

void* kmemcpy(void* dest, const void* src, size_t c) __attribute__((alias("memcpy")));
