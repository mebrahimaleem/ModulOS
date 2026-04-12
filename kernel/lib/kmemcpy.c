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
	const uint8_t* p1 = src;
	uint8_t* p2 = dest;
	
	for (size_t i = 0; i < c; i++) {
		*p2 = *p1;
		p1++;
		p2++;
	}

	return dest;
}

void* kmemcpy(void* dest, const void* src, size_t c) __attribute__((alias("memcpy")));
