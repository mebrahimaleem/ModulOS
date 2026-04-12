/* kmemset.c - library memset implementation */
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

#include <stdint.h>
#include <stddef.h>

#include <lib/kmemset.h>

void* memset(void* ptr, uint64_t v, size_t c) __attribute__((weak));

void* memset(void* ptr, uint64_t v, size_t c) {
	uint8_t* p = ptr;

	for (size_t i = 0; i < c; i++) {
		p[i] = (uint8_t)v;
	}

	return ptr;
}

void* kmemset(void* ptr, uint64_t v, size_t c) __attribute__((alias("memset")));
